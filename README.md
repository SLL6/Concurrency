# concurrency-lab

Scaffold for the from-scratch problems in the Verkada prep plan. Stubs + stress
tests are written; **you write the implementations.** Tests currently FAIL by
design — that's your starting state.

## Setup

**Linux:** `sudo apt install clang make` (or use g++ ≥ 11, already fine)
**macOS:** `xcode-select --install`. Apple Clang supports `-fsanitize=thread`.
Apple Silicon is a bonus: ARM's weak memory model exposes missing barriers that
x86 silently forgives.
**Windows:** WSL2 + the Linux steps. Or Docker: `docker run -it -v $PWD:/w -w /w ubuntu:24.04`
then `apt update && apt install -y clang make`.

Default compiler is `clang++`. Use g++ with `make CXX=g++ ...`.

## Use

```
make tsan               # ThreadSanitizer  — your primary grader
make asan               # Address + UB     — cannot be combined with tsan
make opt                # -O2, no sanitizer — optimizer exposes missing barriers
make run                # build + run everything, all three modes

make tsan T=seqlock && ./build/tsan/seqlock    # one problem
make clean
```

Always run all three. A lock-free bug that hides at `-O0` shows up at `-O2`.

## Order

1. `bounded_queue.hpp` — ingestion path. LC 1188 is the same problem.
2. `sample_store.hpp` — **the reference problem.** Ring buffer + binary search
   + pruning. Build it lock-based and correct first.
3. `thread_pool.hpp` — composes with #1 (a pool is a queue + workers).
4. `seqlock.hpp` — the differentiator. Do this after Williams ch. 5 + preshing.

## The loop, per problem

1. Write the **interface** first — the header, before any implementation.
2. Implement **cold**, no references open. Struggling is the point.
3. Run under `tsan`, `asan`, and `opt`. Green on all three.
4. **Break it deliberately:** downgrade acquire/release to relaxed, remove a
   barrier, widen the lock scope. *Predict the failure first*, then verify.
5. Only now compare against Williams / preshing. Write down every difference
   and why.
6. Re-write cold 2–3 days later. Under ~20 min or it isn't learned.

Narrate out loud while you code — communication is on the rubric.

## Notes

- `harness.hpp` has `lab::stress(...)` (N writers, M readers, timed) and
  `lab::TornCheck` — a multi-field payload whose fields have a fixed
  relationship, so torn reads are *detectable*. A single-int payload can't tear
  visibly and would let a broken seqlock pass.
- **TSan will flag a correct seqlock**, because the unguarded data read is a
  race by the strict definition. Annotate it or make the payload atomic bytes.
  Knowing this is a strong interview signal — say it out loud.
- Check the ARM codegen: paste your seqlock into godbolt.org, target ARM64,
  watch `dmb ish` barriers appear where x86 emitted nothing.
