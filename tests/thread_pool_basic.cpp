// Tests for BasicThreadPool, ordered easiest -> hardest.
// Implement worker_loop/enqueue/shutdown until all seven pass under
// `make tsan`, `make asan`, AND `make opt`.
#include "harness.hpp"
#include "thread_pool_basic.hpp"

#include <atomic>
#include <chrono>
#include <set>
#include <stdexcept>
#include <thread>

using namespace std::chrono_literals;

// 1. Does anything run at all?
static void test_runs_tasks() {
    std::printf("  [1] tasks actually run\n");
    std::atomic<int> counter{0};
    {
        BasicThreadPool pool(4);
        for (int i = 0; i < 1000; ++i)
            pool.enqueue([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
        pool.shutdown();   // drain-or-abandon: see note in test 5
    }
    LAB_CHECK(counter.load() > 0, "no tasks ran at all -- is worker_loop() implemented?");
}

// 2. Exactly once -- no lost tasks, no double execution.
static void test_exactly_once() {
    std::printf("  [2] each task runs exactly once\n");
    constexpr int N = 2000;
    std::atomic<long> sum{0};
    long expected = 0;
    {
        BasicThreadPool pool(4);
        for (int i = 1; i <= N; ++i) {
            expected += i;
            pool.enqueue([&sum, i] { sum.fetch_add(i, std::memory_order_relaxed); });
        }
        pool.shutdown();
    }
    LAB_CHECK(sum.load() == expected,
              "sum mismatch -- tasks were lost or ran more than once");
}

// 3. Real parallelism: work must be spread across threads, not run on one.
static void test_uses_multiple_threads() {
    std::printf("  [3] work is spread across workers\n");
    std::mutex ids_m;
    std::set<std::thread::id> ids;
    {
        BasicThreadPool pool(4);
        for (int i = 0; i < 200; ++i) {
            pool.enqueue([&] {
                std::this_thread::sleep_for(1ms);   // hold the worker briefly
                std::lock_guard<std::mutex> lk(ids_m);
                ids.insert(std::this_thread::get_id());
            });
        }
        pool.shutdown();
    }
    LAB_CHECK(ids.size() > 1,
              "only ONE thread ran tasks -- are you running the task while "
              "still holding the mutex?");
}

// 4. A throwing task must not kill the worker or the process.
static void test_exception_safety() {
    std::printf("  [4] a throwing task doesn't kill the pool\n");
    std::atomic<int> after{0};
    {
        BasicThreadPool pool(2);
        pool.enqueue([] { throw std::runtime_error("boom"); });
        std::this_thread::sleep_for(50ms);
        for (int i = 0; i < 100; ++i)
            pool.enqueue([&after] { after.fetch_add(1, std::memory_order_relaxed); });
        pool.shutdown();
    }
    LAB_CHECK(after.load() > 0,
              "pool stopped working after a task threw -- catch around the invoke");
}

// 5. Shutdown semantics. Whichever you chose, it must be CONSISTENT.
static void test_shutdown_semantics() {
    std::printf("  [5] shutdown is consistent (drain or abandon -- your choice)\n");
    std::atomic<int> ran{0};
    constexpr int N = 500;
    {
        BasicThreadPool pool(2);
        for (int i = 0; i < N; ++i)
            pool.enqueue([&ran] {
                std::this_thread::sleep_for(200us);
                ran.fetch_add(1, std::memory_order_relaxed);
            });
        pool.shutdown();

        int done = ran.load();
        // After shutdown() RETURNS, all workers are joined -- so the count must
        // be stable and must be either "everything" (drain) or "some" (abandon),
        // never still climbing.
        std::this_thread::sleep_for(50ms);
        LAB_CHECK(ran.load() == done,
                  "tasks still running AFTER shutdown() returned -- did you join?");
        std::printf("      -> ran %d/%d (%s)\n", done, N,
                    done == N ? "DRAIN" : "ABANDON");
    }
}

// 6. enqueue() after shutdown must be rejected, not crash or hang.
static void test_enqueue_after_shutdown() {
    std::printf("  [6] enqueue after shutdown is rejected\n");
    BasicThreadPool pool(2);
    pool.shutdown();
    std::atomic<int> ran{0};
    bool accepted = pool.enqueue([&ran] { ran.fetch_add(1); });
    LAB_CHECK(!accepted, "enqueue() returned true after shutdown()");
    std::this_thread::sleep_for(50ms);
    LAB_CHECK(ran.load() == 0, "a task ran after shutdown()");
    pool.shutdown();   // idempotent -- second call must not hang or crash
}

// 7. Destructor with work still queued must not hang. If this test never
//    returns, you have a lost-wakeup or a missing join.
static void test_destructor_no_deadlock() {
    std::printf("  [7] destructor with pending work doesn't hang\n");
    std::atomic<int> ran{0};
    {
        BasicThreadPool pool(3);
        for (int i = 0; i < 1000; ++i)
            pool.enqueue([&ran] { ran.fetch_add(1, std::memory_order_relaxed); });
        // no explicit shutdown -- the destructor must handle it
    }
    std::printf("      -> destructor returned cleanly (%d ran)\n", ran.load());
}

// 8. Concurrent producers hammering enqueue() from many threads.
static void test_concurrent_producers() {
    std::printf("  [8] many threads enqueueing at once\n");
    std::atomic<long> ran{0};
    {
        BasicThreadPool pool(4);
        auto res = lab::stress(
            /*writers=*/4, /*readers=*/0, /*ms=*/800,
            [&](int, const std::atomic<bool>& stop, long& n) {
                while (!stop.load(std::memory_order_relaxed)) {
                    if (!pool.enqueue([&ran] { ran.fetch_add(1, std::memory_order_relaxed); }))
                        break;
                    ++n;
                }
            },
            [](int, const std::atomic<bool>&, long&) {});
        pool.shutdown();
        LAB_CHECK(res.writes > 0, "no tasks enqueued");
        std::printf("      -> enqueued %ld, ran %ld\n", res.writes, ran.load());
    }
}

int main() {
    std::printf("thread_pool_basic\n");
    test_runs_tasks();
    test_exactly_once();
    test_uses_multiple_threads();
    test_exception_safety();
    test_shutdown_semantics();
    test_enqueue_after_shutdown();
    test_destructor_no_deadlock();
    test_concurrent_producers();
    return lab::report("thread_pool_basic");
}
