// harness.hpp — minimal stress harness for concurrent data structures.
//
// Philosophy: a passing single-threaded unit test proves almost nothing about
// a concurrent structure. What catches real bugs is (a) many threads hammering
// it for seconds, (b) an INVARIANT checked on every single read, and (c) the
// whole thing run under ThreadSanitizer.
//
// The invariant is the part people get wrong. If your payload is a single int,
// a torn read is invisible -- any value you observe looks plausible. Use a
// payload with a RELATIONSHIP between fields (see TornCheck below) so that a
// half-old/half-new read is detectable.

#pragma once
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace lab {

// ---------- reporting ----------

inline std::atomic<long> g_failures{0};

inline void fail(const std::string& what) {
    g_failures.fetch_add(1, std::memory_order_relaxed);
    std::fprintf(stderr, "  !! INVARIANT VIOLATED: %s\n", what.c_str());
}

#define LAB_CHECK(cond, msg) do { if (!(cond)) ::lab::fail(msg); } while (0)

inline int report(const char* name) {
    long f = g_failures.load();
    if (f == 0) { std::printf("  [PASS] %s\n", name); return 0; }
    std::printf("  [FAIL] %s -- %ld violation(s)\n", name, f);
    return 1;
}

// ---------- the stress runner ----------
//
// Spins up `n_writers` writer threads and `n_readers` reader threads, runs them
// for `ms` milliseconds, then joins. Each fn receives its own index and a stop
// flag it must poll.

struct StressResult { long writes; long reads; };

inline StressResult stress(int n_writers,
                           int n_readers,
                           int ms,
                           std::function<void(int, const std::atomic<bool>&, long&)> writer,
                           std::function<void(int, const std::atomic<bool>&, long&)> reader) {
    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;
    std::vector<long> wcounts(n_writers, 0), rcounts(n_readers, 0);

    for (int i = 0; i < n_writers; ++i)
        threads.emplace_back([&, i] { writer(i, stop, wcounts[i]); });
    for (int i = 0; i < n_readers; ++i)
        threads.emplace_back([&, i] { reader(i, stop, rcounts[i]); });

    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    stop.store(true, std::memory_order_relaxed);
    for (auto& t : threads) t.join();

    StressResult r{0, 0};
    for (long c : wcounts) r.writes += c;
    for (long c : rcounts) r.reads  += c;
    std::printf("  writes=%ld reads=%ld\n", r.writes, r.reads);
    return r;
}

// ---------- a payload that makes tearing VISIBLE ----------
//
// Three fields with a fixed relationship. If a reader ever observes fields from
// two different writes, the relationship breaks and you catch it. Padding it out
// makes tearing more likely to actually occur (a struct that fits in one word
// may be written atomically by the hardware and never tear at all -- which would
// let a broken implementation pass).

struct TornCheck {
    long a = 0;
    long b = 0;
    long pad[6] = {};   // widen it so a single store can't cover the struct
    long c = 0;

    static TornCheck make(long i) { return TornCheck{i, i * 2, {}, i * 3}; }
    bool consistent() const { return b == a * 2 && c == a * 3; }
};

}  // namespace lab
