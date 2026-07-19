// thread_pool.hpp -- PROBLEM 4: fixed-size thread pool with futures.
//
// Reported as fair game. Note it COMPOSES with problem 1: a thread pool is
// essentially a bounded/unbounded blocking queue plus N workers. Build the
// queue first, then this on top.
//
// Requirements:
//   - N worker threads pulling std::function<void()> off a task queue
//   - submit(f, args...) returns std::future<result_of_t<...>>
//   - clean shutdown: decide DRAIN (finish queued tasks) vs ABANDON (drop them)
//     and be able to argue for one. Destructor must not deadlock or detach.
//
// Discussion points:
//   - How do you size the pool? (hardware_concurrency; CPU- vs IO-bound)
//   - What happens if a task throws? (packaged_task captures it into the future)
//   - Backpressure: if the queue is bounded and full, does submit() block, drop,
//     or fail? For a real-time camera pipeline, which is right -- and why is
//     "block the producer" often the WRONG answer there?
//   - Why is a single shared queue a contention point, and what's the fix?
//     (per-worker queues + work stealing -- know the name, discuss the tradeoff)

#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t n = std::thread::hardware_concurrency()) {
        (void)n;
        // TODO: spawn n workers running worker_loop()
    }

    ~ThreadPool() {
        // TODO: signal stop, notify_all, join all. Must not deadlock.
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;
        // TODO:
        //   wrap in packaged_task<R()>, take its future, push a
        //   std::function<void()> that invokes it, notify one worker,
        //   return the future. Watch out: packaged_task is move-only, so
        //   capturing it in a std::function needs a shared_ptr trick.
        (void)f;
        std::promise<R> p;
        if constexpr (std::is_void_v<R>) p.set_value();
        else p.set_value(R{});
        return p.get_future();  // placeholder
    }

private:
    void worker_loop() {
        // TODO: loop { wait for task or stop; pop; run }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex m_;
    std::condition_variable cv_;
    bool stop_ = false;
};
