

#include "jobsys/job_system.hpp"
#include "render/api/graphics_context.hpp"

namespace mam
{
  thread_local int JobSystem::tls_worker_id_ = -1;

  void JobSystem::Startup(unsigned workers, size_t deque_capacity)
  {
	  Shutdown();
    
	  if (workers == 0) workers = 1;

	  workers_.resize(workers);

	  for (unsigned i = 0; i < workers; ++i)
	  {
		  workers_[i] = std::make_unique<Worker>();
		  workers_[i]->deque = std::make_unique<StealingJobQueue>(deque_capacity);
	  }
	  running_.store(true, std::memory_order_release);
	  for (unsigned i = 0; i < workers; ++i)
	  {
		  workers_[i]->thread = std::thread(&JobSystem::worker_loop, this, i);
	  }
  }

  void JobSystem::Shutdown()
  {
    if (!running_.exchange(false)) return;

    cv_.notify_all();

    for (auto& w : workers_)
    {
      if (w && w->thread.joinable())
      w->thread.join(); 
    }
    workers_.clear();
  }

  JobHandle JobSystem::submit(JobProc fn, void* user, const std::vector<JobHandle>& prerequisites)
  {
    auto handle = submit_callable([fn, user, prerequisites]() {
      for (const auto& pre : prerequisites) { if (pre.valid()) pre.wait(); }
      fn(user);
      });

    return handle;
  }

  void JobSystem::wait_all(const std::vector<JobHandle>& hs) const
  {
    if (hs.empty()) return;

		unsigned spin = 0;
		while (true) {
			bool all_done = true;
      for (auto& h : hs) if (h.valid() && h.f_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        all_done = false;
        break;
      }
			if (all_done) break;

      if (++spin < 100) std::this_thread::yield();
      else std::this_thread::sleep_for(std::chrono::microseconds(50));
		}
  }

  void JobSystem::worker_loop(unsigned worker_id)
  {
    tls_worker_id_ = static_cast<int>(worker_id);

    while (running_.load(std::memory_order_acquire)) {
      Job job;

      if (workers_[worker_id]->deque->steal(job)) {
        if (job.fn) job.fn();
        continue;
      }

      bool stole = false;
      for (size_t i = 0; i < workers_.size(); ++i) {
        if (i == worker_id) continue;
        if (workers_[i]->deque->steal(job)) {
          steal_success_count_.fetch_add(1, std::memory_order_relaxed);
          if (job.fn) job.fn();
          stole = true;
          break;
        }
      }

      if (!stole) {
        idle_wakeups_.fetch_add(1, std::memory_order_relaxed);
        std::unique_lock<std::mutex> lock(cv_mtx_);
        cv_.wait_for(lock, std::chrono::microseconds(100), [&] {
          return !running_.load(std::memory_order_relaxed)
            || !workers_[worker_id]->deque->empty();
          });
      }
    }

    tls_worker_id_ = -1;
  }

  size_t JobSystem::random_worker_id() const
  {
    if (workers_.empty()) return 0;
    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<size_t> dist(0, workers_.size() - 1);
    return dist(rng);
  }

  bool  StealingJobQueue::push(Job job)
  {
    const size_t b = bottom_.load(std::memory_order_relaxed);
    const size_t t = top_.load(std::memory_order_acquire);

    if (b - t >= capacity_) {
      return false;
    }

    buffer_[b & mask_].j = std::move(job);
    std::atomic_thread_fence(std::memory_order_release);
    bottom_.store(b + 1, std::memory_order_relaxed);
    return true;
  }

  bool StealingJobQueue::pop(Job& job)
  {
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_relaxed);
    if (b == t) return false;  

    b -= 1;
    bottom_.store(b, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    t = top_.load(std::memory_order_acquire);

    if (t <= b) {
      job = buffer_[b & mask_].j;

      if (t == b) {
        if (!top_.compare_exchange_strong(t, t + 1,
          std::memory_order_seq_cst,
          std::memory_order_relaxed)) {
          job = Job{};
          bottom_.store(b + 1, std::memory_order_relaxed);
          return false;
        }
        bottom_.store(b + 1, std::memory_order_relaxed);
      }
      return true;
    }
    else {
      bottom_.store(b + 1, std::memory_order_relaxed);
      return false;
    }
  }

  bool StealingJobQueue::steal(Job& job)
  {
	  size_t t = top_.load(std::memory_order_acquire);
	  std::atomic_thread_fence(std::memory_order_acquire);
	  size_t b = bottom_.load(std::memory_order_acquire);

    if (t >= b) {
			return false; 
    }

    job = buffer_[t & mask_].j;
    if (!top_.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
			return false; 
    }
    return true;
  }

  bool StealingJobQueue::empty() const
  {    
    return top_.load(std::memory_order_relaxed) >= bottom_.load(std::memory_order_relaxed);
  }

}
