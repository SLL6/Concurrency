// Stress: N producers, M consumers. Every item pushed must be popped exactly
// once -- so the sum of what comes out must equal the sum of what went in.
#include "harness.hpp"
#include "bounded_queue.hpp"
#include <atomic>

int main() {
    std::printf("bounded_queue: %d producers / %d consumers\n", 3, 3);

    BoundedQueue<long> q(64);
    std::atomic<long> pushed_sum{0}, popped_sum{0};

    auto res = lab::stress(
        /*writers=*/3, /*readers=*/3, /*ms=*/1500,
        [&](int id, const std::atomic<bool>& stop, long& n) {
            long v = id * 1'000'000L;
            while (!stop.load(std::memory_order_relaxed)) {
                if (!q.push(++v)) break;
                pushed_sum.fetch_add(v, std::memory_order_relaxed);
                ++n;
            }
        },
        [&](int, const std::atomic<bool>& stop, long& n) {
            while (!stop.load(std::memory_order_relaxed)) {
                auto v = q.pop();
                if (!v) break;
                popped_sum.fetch_add(*v, std::memory_order_relaxed);
                ++n;
            }
        });

    q.close();
    // Drain whatever is left so the accounting balances.
    while (auto v = q.pop()) popped_sum.fetch_add(*v, std::memory_order_relaxed);

    LAB_CHECK(res.writes > 0, "no pushes completed -- is push() implemented?");
    LAB_CHECK(res.reads  > 0, "no pops completed -- is pop() implemented?");
    LAB_CHECK(pushed_sum.load() == popped_sum.load(),
              "items lost or duplicated (pushed_sum != popped_sum)");

    return lab::report("bounded_queue");
}
