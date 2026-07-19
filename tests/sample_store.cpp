// Stress: one writer ingesting a monotonically-timestamped stream, many readers
// querying latest() / at() / window(). Invariants:
//   - latest() is always valid and never goes BACKWARDS in time for a reader
//   - at(t) returns a sample with t_us <= t
//   - size() never exceeds capacity  (bounded memory -- the rubric cares)
#include "harness.hpp"
#include "sample_store.hpp"
#include <thread>

int main() {
    constexpr std::size_t CAP = 1024;
    std::printf("sample_store: 1 writer / 4 readers, capacity=%zu\n", CAP);

    SampleStore store(CAP);

    auto res = lab::stress(
        /*writers=*/1, /*readers=*/4, /*ms=*/1500,
        [&](int, const std::atomic<bool>& stop, long& n) {
            std::uint64_t t = 1;
            while (!stop.load(std::memory_order_relaxed)) {
                store.ingest(Sample{t, static_cast<double>(t) * 0.5, true});
                t += 10;
                ++n;
            }
        },
        [&](int, const std::atomic<bool>& stop, long& n) {
            std::uint64_t last_seen = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                if (auto s = store.latest()) {
                    LAB_CHECK(s->valid, "latest() returned an INVALID sample");
                    LAB_CHECK(s->t_us >= last_seen, "latest() went BACKWARDS in time");
                    last_seen = s->t_us;

                    if (auto a = store.at(s->t_us)) {
                        LAB_CHECK(a->t_us <= s->t_us, "at(t) returned a sample AFTER t");
                    }
                    auto w = store.window(s->t_us > 500 ? s->t_us - 500 : 0, s->t_us);
                    for (auto& x : w)
                        LAB_CHECK(x.t_us <= s->t_us, "window() returned out-of-range sample");
                }
                LAB_CHECK(store.size() <= CAP, "size() EXCEEDED capacity -- unbounded growth");
                ++n;
            }
        });

    LAB_CHECK(res.writes > 0, "no ingests -- implemented?");
    LAB_CHECK(res.reads  > 0, "no reads -- implemented?");
    return lab::report("sample_store");
}
