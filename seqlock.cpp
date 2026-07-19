// Stress: ONE writer, many readers. The payload's fields have a fixed
// relationship (b == 2a, c == 3a). If a reader ever observes fields from two
// different writes, the relationship breaks -- that's a torn read.
//
// This is the whole point of TornCheck: a single-int payload cannot show
// tearing, so it would let a broken seqlock pass.
#include "harness.hpp"
#include "seqlock.hpp"

int main() {
    std::printf("seqlock: 1 writer / 6 readers, checking for torn reads\n");

    SeqLock<lab::TornCheck> sl;
    sl.write(lab::TornCheck::make(1));

    auto res = lab::stress(
        /*writers=*/1, /*readers=*/6, /*ms=*/1500,
        [&](int, const std::atomic<bool>& stop, long& n) {
            long i = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                sl.write(lab::TornCheck::make(++i));
                ++n;
            }
        },
        [&](int, const std::atomic<bool>& stop, long& n) {
            while (!stop.load(std::memory_order_relaxed)) {
                lab::TornCheck v = sl.read();
                LAB_CHECK(v.consistent(), "TORN READ: fields from different writes");
                ++n;
            }
        });

    LAB_CHECK(res.writes > 0 && res.reads > 0, "no traffic -- implemented?");

    // Exercise: once this passes, downgrade every acquire/release to relaxed
    // and re-run under `make opt` (-O2) and on ARM. Predict the failure first.
    return lab::report("seqlock");
}
