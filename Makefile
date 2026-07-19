# Concurrency lab — build every test three ways.
#
#   make tsan   -> ThreadSanitizer  (races; your primary grader)
#   make asan   -> Address+UB       (memory bugs; cannot combine with tsan)
#   make opt    -> -O2, no sanitizer(optimizer exposes missing barriers)
#   make all    -> all three
#   make run    -> build + run everything
#   make clean
#
# Single test:  make tsan T=seqlock && ./build/tsan/seqlock

CXX      ?= clang++
STD      := -std=c++20
WARN     := -Wall -Wextra -Wpedantic
BASE     := $(STD) $(WARN) -g -pthread -Iinclude

TSAN_FLAGS := -fsanitize=thread -O1
ASAN_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -O1
OPT_FLAGS  := -O2

SRCS  := $(wildcard tests/*.cpp)
NAMES := $(notdir $(basename $(SRCS)))
T     ?= $(NAMES)

.PHONY: all tsan asan opt run clean
all: tsan asan opt

tsan: $(addprefix build/tsan/,$(T))
asan: $(addprefix build/asan/,$(T))
opt:  $(addprefix build/opt/,$(T))

build/tsan/%: tests/%.cpp $(wildcard include/*.hpp)
	@mkdir -p $(dir $@)
	$(CXX) $(BASE) $(TSAN_FLAGS) $< -o $@

build/asan/%: tests/%.cpp $(wildcard include/*.hpp)
	@mkdir -p $(dir $@)
	$(CXX) $(BASE) $(ASAN_FLAGS) $< -o $@

build/opt/%: tests/%.cpp $(wildcard include/*.hpp)
	@mkdir -p $(dir $@)
	$(CXX) $(BASE) $(OPT_FLAGS) $< -o $@

run: all
	@for m in tsan asan opt; do \
	  for t in $(T); do \
	    printf "\n=== %s / %s ===\n" $$m $$t; \
	    ./build/$$m/$$t || echo "FAILED: $$m/$$t"; \
	  done; \
	done

clean:
	rm -rf build
