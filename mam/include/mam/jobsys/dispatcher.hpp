
#pragma once

#include "common/common.hpp"
#include <mutex>
#include <vector>
#include <functional>

namespace mam {

  /**
   * @brief Mutex protecting the main thread queue.
   */
  inline std::mutex g_mainMtx;

  /**
   * @brief Queue of functions to run on the main thread.
   */
  inline std::vector<std::function<void()>> g_mainQueue;

  /**
   * @brief Enqueue a function to be run on the main thread.
   *
   * Thread-safe: locks the main mutex before adding to the queue.
   * @param f Function to enqueue
   */
  inline void EnqueueToMain(std::function<void()> f) {
    std::lock_guard<std::mutex> lk(g_mainMtx);
    g_mainQueue.push_back(std::move(f));
  }

  /**
   * @brief Drain and execute all functions queued for the main thread.
   *
   * Should be called periodically from the main thread.
   * Swaps the queue with a temporary vector to minimize lock time.
   */
  inline void DrainMainQueue() {
    std::vector<std::function<void()>> tmp;
    {
      std::lock_guard<std::mutex> lk(g_mainMtx);
      tmp.swap(g_mainQueue);
    }
    for (auto& f : tmp) f();
  }

  /**
   * @brief Dispatcher class for enqueuing functions to the main thread.
   *
   * Provides a static template method to enqueue functions with arguments.
   */
  class Dispatcher
  {
  public:
    /**
     * @brief Enqueue a function with arguments to run on the main thread.
     *
     * Example usage:
     * @code
     * Dispatcher::RunOnMain([](int x){ std::cout << x; }, 42);
     * @endcode
     *
     * @tparam F Function type
     * @tparam Args Argument types
     * @param f Function to run
     * @param args Arguments to pass to the function
     */
    template<typename F, typename... Args>
    static void RunOnMain(F&& f, Args&&... args)
    {
      EnqueueToMain(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    }
  };

} // namespace mam