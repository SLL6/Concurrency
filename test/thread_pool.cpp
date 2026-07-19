// Tests: every submitted task runs exactly once, futures deliver correct values,
// exceptions propagate through the future, and the destructor doesn't deadlock.
#include "harness.hpp"
#include "thread_pool.hpp"
#include <atomic>
#include <stdexcept>

int main() {
    std::printf("thread_pool: 2000 tasks across hardware_concurrency workers\n");

    constexpr int N = 2000;
    std::atomic<int> ran{0};
    long expected = 0;

    {
        ThreadPool pool(4);
        std::vector<std::future<long>> futs;
        futs.reserve(N);

        for (int i = 0; i < N; ++i) {
            expected += i;
            futs.push_back(pool.submit([i, &ran] {
                ran.fetch_add(1, std::memory_order_relaxed);
                return static_cast<long>(i);
            }));
        }

        long sum = 0;
        for (auto& f : futs) sum += f.get();

        LAB_CHECK(sum == expected, "future results wrong -- tasks lost or duplicated");
        LAB_CHECK(ran.load() == N, "task ran count != N");

        // Exceptions must travel through the future, not terminate the worker.
        auto bad = pool.submit([]() -> int { throw std::runtime_error("boom"); });
        bool caught = false;
        try { bad.get(); } catch (const std::exception&) { caught = true; }
        LAB_CHECK(caught, "exception did not propagate through the future");

        // Submit more, then let the destructor run -- it must not hang.
        for (int i = 0; i < 100; ++i) pool.submit([] { });
    }  // <- destructor: drain or abandon? Decide, and be able to defend it.

    std::printf("  destructor completed without deadlock\n");
    return lab::report("thread_pool");
}
