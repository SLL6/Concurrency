// thread_pool_basic.hpp -- PROBLEM 4a: thread pool, no templates, no futures.
//
// Strip away the generic machinery and a thread pool is just:
//     a queue of work  +  N threads looping on it  +  a way to stop cleanly
//
// Tasks are plain `std::function<void()>`. If a caller wants a result back,
// they capture their own promise/atomic in the lambda. That's a fine design --
// build the templated submit() later, once THIS is second nature.
//
// ---------------------------------------------------------------------------
// WHAT YOU IMPLEMENT (each marked TODO):
//   ctor          -- spawn n worker threads running worker_loop()
//   enqueue       -- push a task, wake one worker; return false if stopping
//   worker_loop   -- wait for work or stop; pop; run OUTSIDE the lock
//   shutdown      -- signal stop, wake everyone, join all
//   dtor          -- call shutdown(); must never hang or std::terminate
//
// ---------------------------------------------------------------------------
// THE FIVE THINGS THAT GO WRONG (get these right and you're done):
//
// 1. Waiting without a predicate. `cv_.wait(lk)` alone is a bug -- spurious
//    wakeups are real. Always `cv_.wait(lk, [this]{ return stop_ || !tasks_.empty(); })`.
//
// 2. Running the task while holding the lock. Pop under the lock, UNLOCK, then
//    invoke. Otherwise your pool is a very expensive single thread.
//
// 3. Shutdown that loses the wakeup. Set stop_ WHILE holding the mutex, then
//    notify_all(). Setting it outside the lock races with a worker that has
//    just checked the predicate and is about to sleep -- it sleeps forever.
//
// 4. Destructor that deadlocks or terminates. A std::thread destroyed while
//    joinable calls std::terminate. Join every worker. And never join from
//    inside a worker (self-join -> deadlock).
//
// 5. A throwing task killing a worker. An exception escaping worker_loop()
//    calls std::terminate and takes the process down. Catch around the invoke.
//
// ---------------------------------------------------------------------------
// DECIDE AND BE ABLE TO DEFEND:
//   - DRAIN (run everything queued) vs ABANDON (drop pending) on shutdown?
//     Both are legitimate. Pick one, say why. Hint: for a camera pipeline
//     shutting down, is finishing 4000 queued frames the right behavior?
//   - notify_one on enqueue vs notify_all -- when would one be wrong?
//   - Unbounded queue = unbounded memory. What's the backpressure story if
//     producers outrun workers? (bound it -> then does enqueue block, drop
//     oldest, or fail? For real-time ingest, blocking the producer is often
//     the WRONG answer -- know why.)
//   - Pool sizing: hardware_concurrency() for CPU-bound; more for IO-bound. Why?
//   - One shared queue is a contention point. What's the fix, and its cost?
//     (per-worker queues + work stealing -- know the name and the tradeoff.)

#pragma once
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class BasicThreadPool {
public:
    using Task = std::function<void()>;

    explicit BasicThreadPool(std::size_t n_threads) {
        (void)n_threads;
        // TODO: reserve and spawn n_threads workers, each running worker_loop().
        //       Guard against n_threads == 0 (a pool with no workers silently
        //       never runs anything -- decide: clamp to 1, or document it).
    }

    ~BasicThreadPool() {
        // TODO: shutdown(). Must be idempotent -- shutdown() may already have
        //       been called explicitly.
    }

    BasicThreadPool(const BasicThreadPool&) = delete;
    BasicThreadPool& operator=(const BasicThreadPool&) = delete;

    // Queue a task. Returns false if the pool is shutting down (task not taken).
    bool enqueue(Task t) {
        (void)t;
        // TODO:
        //   lock; if (stop_) return false; tasks_.push(std::move(t));
        //   unlock (or scope it); cv_.notify_one(); return true;
        return false;
    }

    // Stop accepting work, wake all workers, join them. Safe to call twice.
    void shutdown() {
        // TODO:
        //   { lock_guard lk(m_); if (stop_) return; stop_ = true; }   <- inside the lock
        //   cv_.notify_all();
        //   for (auto& w : workers_) if (w.joinable()) w.join();
    }

    // Number of tasks not yet started. Useful for tests; note it's a snapshot.
    std::size_t pending() const {
        std::lock_guard<std::mutex> lk(m_);
        return tasks_.size();
    }

    std::size_t size() const { return workers_.size(); }

private:
    void worker_loop() {
        // TODO:
        //   for (;;) {
        //     Task t;
        //     {
        //       unique_lock lk(m_);
        //       cv_.wait(lk, [this]{ return stop_ || !tasks_.empty(); });
        //
        //       // THIS LINE ENCODES YOUR DRAIN-VS-ABANDON DECISION:
        //       //   abandon: if (stop_) return;
        //       //   drain:   if (stop_ && tasks_.empty()) return;
        //
        //       t = std::move(tasks_.front());
        //       tasks_.pop();
        //     }                      // <-- lock released HERE, before running
        //     try { t(); } catch (...) { /* never let it escape the worker */ }
        //   }
    }

    mutable std::mutex m_;
    std::condition_variable cv_;
    std::queue<Task> tasks_;
    std::vector<std::thread> workers_;
    bool stop_ = false;
};
