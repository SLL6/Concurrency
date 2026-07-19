// bounded_queue.hpp -- PROBLEM 1: thread-safe bounded blocking queue.
//
// This is your ingestion path. LC 1188 is the same problem.
//
// Requirements:
//   - fixed capacity; push() blocks when full, pop() blocks when empty
//   - mutex + TWO condition variables (not_full_, not_empty_)
//   - every cv wait uses the PREDICATE form (spurious wakeups are real)
//   - close() wakes everyone and makes further pops return false once drained
//
// Questions to be able to answer out loud when you're done:
//   - Why two condition variables instead of one + notify_all?
//   - notify_one vs notify_all here -- when is one wrong?
//   - Should you notify while holding the lock, or after unlocking? Tradeoff?
//   - What happens to a blocked producer at shutdown? (that's what close() is for)
//   - How would you add try_push / push_with_timeout?

#pragma once
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : cap_(capacity) {}

    // Blocks while full. Returns false if the queue was closed.
    bool push(T value) {
        (void)value;
        return false;  // TODO: implement
    }

    // Blocks while empty. Returns nullopt if closed AND drained.
    std::optional<T> pop() {
        return std::nullopt;  // TODO: implement
    }

    void close() {
        // TODO: set closed_, notify_all on both cvs
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(m_);
        return q_.size();
    }

private:
    mutable std::mutex m_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    std::queue<T> q_;
    std::size_t cap_;
    bool closed_ = false;
};
