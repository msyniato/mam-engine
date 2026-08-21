#pragma once

#include "common/common.hpp"
#include <thread>
#include <future>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>

namespace mam {

  /**
   * @brief Function-pointer job signature used by JobSystem::submit().
   * @param user Optional user-provided data pointer.
   */
  using JobProc = void(*)(void* user);

  /**
   * @brief Lightweight handle to wait on a submitted job's completion.
   *
   * Wraps a std::shared_future<void> to allow safe copying and deferred waiting.
   */
  class JobHandle {
  public:
    JobHandle() = default;

    /**
     * @brief Construct a handle from a shared future.
     * @param f Shared future representing the job's completion state.
     */
    explicit JobHandle(std::shared_future<void> f) : f_(std::move(f)) {}

    /// Returns true if the handle refers to a valid submitted job.
    bool valid() const { return (bool)f_.valid(); }

    /// Block the calling thread until the job is complete.
    void wait() const { if (f_.valid()) f_.wait(); }

  private:
    std::shared_future<void> f_;
    friend class JobSystem;
  };

  /**
   * @brief A single executable job unit.
   */
  struct Job {
    std::function<void()> fn; ///< Callable to execute on a worker thread.
  };

  /**
   * @brief Lock-free, bounded, work-stealing deque.
   *
   * The owning thread pushes and pops at the bottom.
   * Other threads steal from the top.
   */
  class StealingJobQueue {
  public:
    /**
     * @brief Construct the queue with the given capacity.
     *
     * Capacity is rounded up to the next power of two internally.
     * @param capacity Maximum number of jobs the queue can hold.
     */
    explicit StealingJobQueue(size_t capacity = 1024) :
      capacity_(normalize(capacity)),
      mask_(capacity_ - 1),
      top_(0),
      bottom_(0),
      buffer_(std::make_unique<Node[]>(capacity_))
    {}

    /**
     * @brief Push a job onto the bottom of the deque (owner thread only).
     * @param job Job to enqueue.
     * @return True if the job was accepted; false if the deque is full.
     */
    bool push(Job job);

    /**
     * @brief Pop a job from the bottom of the deque (owner thread only).
     * @param job Receives the popped job on success.
     * @return True if a job was retrieved; false if the deque is empty.
     */
    bool pop(Job& job);

    /**
     * @brief Steal a job from the top of the deque (any thread).
     * @param job Receives the stolen job on success.
     * @return True if a job was stolen; false otherwise.
     */
    bool steal(Job& job);

    /// Returns true if the deque appears empty (approximate).
    bool empty() const;

    /**
     * @brief Returns an approximate count of jobs currently in the deque.
     * @return Approximate number of pending jobs.
     */
    size_t size_approx() const
    {
      size_t t = top_.load(std::memory_order_relaxed);
      size_t b = bottom_.load(std::memory_order_relaxed);
      return (b >= t) ? (b - t) : 0;
    }

  private:
    struct Node { Job j; };

    static size_t normalize(size_t capacity) {
      size_t n = 1;
      while (n < capacity) n <<= 1;
      return n;
    }

    const size_t          capacity_;  ///< Power-of-two capacity of the deque.
    const size_t          mask_;      ///< Bitmask used for index wrapping.
    std::atomic<size_t>   top_;       ///< Top index (steal side).
    std::atomic<size_t>   bottom_;    ///< Bottom index (owner side).
    std::unique_ptr<Node[]> buffer_;  ///< Ring-buffer storage.
  };

  /**
   * @brief Multi-threaded job system with work-stealing workers.
   *
   * Each worker thread owns a StealingJobQueue deque. Jobs are dispatched
   * to a random worker and may be stolen by idle workers for load balancing.
   * Provides submit(), submit_callable(), and wait_all().
   */
  class JobSystem {
  public:
    JobSystem() : running_(false) {}
    ~JobSystem() { Shutdown(); }

    /**
     * @brief Start the job system and spawn worker threads.
     * @param workers        Number of worker threads (default: hardware concurrency).
     * @param deque_capacity Per-worker deque capacity.
     */
    void Startup(unsigned workers = std::thread::hardware_concurrency(), size_t deque_capacity = 1024);

    /// Drain all pending jobs, stop workers, and join all threads.
    void Shutdown();

    /**
     * @brief Submit a C-style job with optional prerequisite handles.
     * @param fn            Function pointer to execute.
     * @param user          Optional user data pointer passed to fn.
     * @param prerequisites Jobs that must complete before this one starts.
     * @return Handle representing the submitted job.
     */
    JobHandle submit(JobProc fn, void* user, const std::vector<JobHandle>& prerequisites = {});

    /**
     * @brief Submit any callable (lambda, std::function, bind expression) as a job.
     * @tparam F    Callable type.
     * @tparam Args Argument types.
     * @param f    Callable to execute.
     * @param args Arguments forwarded to the callable.
     * @return Handle representing the submitted job.
     */
    template<typename F, typename... Args>
    auto submit_callable(F&& f, Args&&... args);

    /**
     * @brief Block until all jobs referenced by the given handles are complete.
     * @param hs Vector of job handles to wait on.
     */
    void wait_all(const std::vector<JobHandle>& hs) const;

    /**
     * @brief Block until the system has no active jobs.
     */
    void wait_idle() const {
      unsigned spin = 0;
      while (active_jobs_.load(std::memory_order_acquire) > 0) {
        if (++spin < 100) std::this_thread::yield();
        else std::this_thread::sleep_for(std::chrono::microseconds(50));
      }
    }

    /// Returns the number of active worker threads.
    unsigned GetWorkerCount()       const { return static_cast<unsigned>(workers_.size()); }

    /// Returns the total number of jobs submitted since startup.
    uint64_t GetSubmittedJobs()     const { return submitted_jobs_.load(std::memory_order_relaxed); }

    /// Returns the total number of jobs completed since startup.
    uint64_t GetCompletedJobs()     const { return completed_jobs_.load(std::memory_order_relaxed); }

    /// Returns the number of jobs executed inline (queue-full fallback).
    uint64_t GetInlineExecutedJobs() const { return inline_executed_jobs_.load(std::memory_order_relaxed); }

    /// Returns the number of successful work-steal operations.
    uint64_t GetStealSuccessCount() const { return steal_success_count_.load(std::memory_order_relaxed); }

    /// Returns the number of jobs currently in flight.
    uint64_t GetActiveJobs()        const { return active_jobs_.load(std::memory_order_relaxed); }

    /// Returns the number of times sleeping workers have been woken up.
    uint64_t GetIdleWakeups()       const { return idle_wakeups_.load(std::memory_order_relaxed); }

    /**
     * @brief Returns the approximate queue depth for a specific worker.
     * @param workerIndex Zero-based worker index.
     * @return Approximate number of pending jobs in that worker's deque.
     */
    size_t GetWorkerQueueSize(size_t workerIndex) const
    {
      if (workerIndex >= workers_.size() || !workers_[workerIndex] || !workers_[workerIndex]->deque)
        return 0;
      return workers_[workerIndex]->deque->size_approx();
    }

  private:
    struct Worker {
      std::thread                        thread; ///< Worker thread.
      std::unique_ptr<StealingJobQueue>  deque;  ///< Work-stealing deque owned by this worker.
    };

    void   worker_loop(unsigned worker_id);
    size_t random_worker_id() const;

    std::atomic<bool>                    running_;   ///< True while the system is active.
    std::vector<std::unique_ptr<Worker>> workers_;   ///< All worker threads and their deques.
    std::condition_variable              cv_;        ///< Condition variable used to wake idle workers.
    std::mutex                           cv_mtx_;    ///< Mutex protecting the condition variable.

    std::atomic<uint64_t> submitted_jobs_{ 0 };
    std::atomic<uint64_t> completed_jobs_{ 0 };
    std::atomic<uint64_t> idle_wakeups_{ 0 };
    std::atomic<uint64_t> inline_executed_jobs_{ 0 };
    std::atomic<uint64_t> steal_success_count_{ 0 };
    std::atomic<uint64_t> active_jobs_{ 0 };

    static thread_local int tls_worker_id_; ///< Thread-local worker index (-1 on non-worker threads).
  };

  /**
   * @brief Template implementation of submit_callable().
   *
   * Wraps the callable in a Job that fulfils a promise on completion,
   * then enqueues it to the current or a random worker's deque.
   */
  template<typename F, typename... Args>
  inline auto JobSystem::submit_callable(F&& f, Args&&... args)
  {
    auto prom = std::make_shared<std::promise<void>>();
    JobHandle handle{ prom->get_future().share() };

    submitted_jobs_.fetch_add(1, std::memory_order_relaxed);
    active_jobs_.fetch_add(1, std::memory_order_relaxed);

    auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    Job j;

    j.fn = [this, prom, task = std::move(bound)]() mutable {
      try {
        task();
        prom->set_value();
      }
      catch (...) {
        prom->set_exception(std::current_exception());
      }

      completed_jobs_.fetch_add(1, std::memory_order_relaxed);
      active_jobs_.fetch_sub(1, std::memory_order_relaxed);
    };

    const int    wi  = tls_worker_id_;
    const size_t idx = (wi >= 0) ? static_cast<size_t>(wi) : random_worker_id();

    if (!workers_[idx]->deque->push(j)) {
      bool enqueued = false;
      for (size_t k = 0; k < workers_.size(); ++k) {
        if (k == idx) continue;
        if (workers_[k]->deque->push(j)) {
          enqueued = true;
          break;
        }
      }

      if (!enqueued) {
        inline_executed_jobs_.fetch_add(1, std::memory_order_relaxed);
        j.fn();
      }

      cv_.notify_one();
    }
    else {
      cv_.notify_one();
    }

    return handle;
  }

} // namespace mam
