// sample_store.hpp -- PROBLEM 2: the reference problem from the recruiter brief.
//
// A thread-safe store for streaming, timestamped samples. One (or few) writers
// ingesting continuously; many readers asking:
//     latest()      -> most recent VALID sample, O(1)
//     at(t)         -> value at-or-before t, via BINARY SEARCH
//     window(lo,hi) -> range query
//
// Backed by a FIXED-CAPACITY ring buffer so memory is bounded, plus optional
// time-window pruning. Closest LeetCode analogues: 981 (Time Based K-V Store),
// 1146 (Snapshot Array) -- both single-threaded; the concurrency is on you.
//
// Design points the rubric is listening for:
//   - Separate the INGEST path from the READ path.
//   - Ring buffer: use power-of-two capacity + masking, not %.
//     Decide how you distinguish full from empty (count? waste a slot?
//     free-running counters you mask on access?). Say which and why.
//   - Samples arrive in time order -- so the buffer is sorted, so at() is a
//     binary search over a wrapped array. Careful with the wrap.
//   - Pruning: drop samples older than now - window_. Bounded memory.
//   - Reject invalid / out-of-window / out-of-order samples cleanly on ingest.
//   - latest() should not block the writer. Start with a mutex; then discuss
//     (or implement) a seqlock or double-buffer for the latest-value fast path.
//
// Build it lock-based and CORRECT first. Narrate where the lock could narrow.

#pragma once
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

struct Sample {
    std::uint64_t t_us = 0;
    double value = 0.0;
    bool valid = false;
};

class SampleStore {
public:
    // capacity MUST be a power of two (assert it). window_us == 0 -> no time pruning.
    explicit SampleStore(std::size_t capacity, std::uint64_t window_us = 0)
        : buf_(capacity), cap_(capacity), mask_(capacity - 1), window_us_(window_us) {
        // TODO: assert power of two
    }

    // Writer path. Hot. Should not be blocked by readers any longer than necessary.
    // Rejects: invalid samples, out-of-order timestamps, samples outside the window.
    bool ingest(const Sample& s) {
        (void)s;
        return false;  // TODO
    }

    // Most recent VALID sample. O(1).
    std::optional<Sample> latest() const {
        return std::nullopt;  // TODO
    }

    // Value at-or-before t. Binary search over the (wrapped) ring. O(log n).
    std::optional<Sample> at(std::uint64_t t_us) const {
        (void)t_us;
        return std::nullopt;  // TODO
    }

    // All samples in [lo, hi]. Binary search for the start, then walk.
    std::vector<Sample> window(std::uint64_t lo, std::uint64_t hi) const {
        (void)lo; (void)hi;
        return {};  // TODO
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(m_);
        return count_;
    }

private:
    // Logical index i (0 = oldest) -> physical slot.
    std::size_t slot(std::size_t i) const { return (head_ + i) & mask_; }

    mutable std::mutex m_;
    std::vector<Sample> buf_;
    std::size_t cap_;
    std::size_t mask_;
    std::size_t head_ = 0;   // oldest
    std::size_t count_ = 0;
    std::uint64_t window_us_ = 0;
};
