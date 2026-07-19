// seqlock.hpp -- PROBLEM 3: sequence lock (single writer, many readers).
//
// The "read the most recent value without ever blocking the writer" primitive.
// No LeetCode equivalent -- your grader here is TSan + the torn-read stress test.
//
// Shape:
//   writer: seq++ (now ODD) ... write data ... seq++ (now EVEN)
//   reader: s0 = seq; if odd -> retry; copy data; s1 = seq; if s0 != s1 -> retry
//
// The ordering is the whole problem:
//   - release fence AFTER bumping to odd, BEFORE touching data
//   - release store when bumping back to even
//   - acquire load of s0; acquire fence AFTER copying, BEFORE loading s1
// Get these wrong and it still passes on x86 and breaks on ARM. Compile for
// ARM64 on godbolt and watch the dmb barriers appear.
//
// Constraints to be able to state:
//   - single writer only (multiple writers need a mutex among themselves)
//   - payload must be trivially copyable -- a reader CAN see a torn value, and
//     copying a torn std::string or dereferencing a torn pointer is UB, not a
//     retry. You only get to discard the garbage if reading it was harmless.
//   - readers can starve under a hot writer (inverse of a rwlock)
//   - large payloads are bad: every retry re-copies everything
//
// TSan note: it will flag the unguarded data read as a race, because it IS one
// by the strict definition. Either annotate, or make the payload atomic bytes.
// Knowing this is a strong interview signal -- mention it.

#pragma once
#include <atomic>
#include <cstdint>
#include <type_traits>

template <typename T>
class SeqLock {
    static_assert(std::is_trivially_copyable_v<T>,
                  "seqlock payload must be trivially copyable -- readers can observe torn state");

public:
    SeqLock() = default;

    // Single writer only.
    void write(const T& v) {
        (void)v;
        // TODO:
        //   1. load seq_ (relaxed), store seq_+1 (relaxed)  -> odd
        //   2. atomic_thread_fence(release)
        //   3. data_ = v
        //   4. store seq_+2 (release)                        -> even
    }

    T read() const {
        T out{};
        // TODO:
        //   do {
        //     s0 = seq_.load(acquire);
        //     if (s0 & 1) { retry; }
        //     out = data_;
        //     atomic_thread_fence(acquire);
        //     s1 = seq_.load(relaxed);
        //   } while (s0 != s1);
        return out;
    }

private:
    mutable std::atomic<std::uint64_t> seq_{0};
    T data_{};
};
