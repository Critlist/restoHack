# restoHack Modular Test Suite

The old monolithic `gauntlet_v3.py` has been broken down into focused, modular tools:

## Individual Test Runners

Run specific sanitizers individually:

```bash
# ASan only (clean rebuild by default)
./run-asan.py --runs 25 --steps 50

# UBSan only with debug and enhanced testing
./run-ubsan.py --debug --enhanced

# ASan+UBSan combined (build-only mode)
./run-asan-ubsan.py --build-only

# Skip clean rebuild for faster iteration (may have stale objects)
./run-asan.py --no-clean --test-only

# Standalone fuzzer (see "Fuzzer" below; there is no run-fuzz.py)
./standalone-fuzzer.py --duration 120
```

## Fuzzer

`standalone-fuzzer.py` builds small libFuzzer targets with
`clang -fsanitize=fuzzer,address` and runs each one:

```bash
./standalone-fuzzer.py --duration 120      # all targets, 120s each (default 30)
./standalone-fuzzer.py --target integers   # one target (substring match on name)
./standalone-fuzzer.py --debug             # verbose output
```

The targets are self-contained C reimplementations compiled one file at a
time. They do not link or exercise anything from `src/` (the game code), so a
clean fuzz run says nothing about restoHack itself.

## Reproducible Runs

Each run gets a fixed seed (printed as `seed=N` on failure). Lane builds are
compiled with `-DHACK_TEST_SEED`, which makes the game seed its RNG from the
`HACK_SEED` environment variable, and the harness seeds its own key choices
with the same number. Rerunning the same lane and run number therefore replays
the same dungeon and the same keystrokes. To open a failing seed by hand:

```bash
HACK_SEED=<N> ../build-asan-ubsan/hack -D
```

Release builds don't define `HACK_TEST_SEED` and ignore `HACK_SEED`.
Date-dependent events (moon phase, Friday 13th, midnight) still vary by day.

## Orchestration Script

Run multiple tests in sequence:

```bash
# Default: Run all sanitizers (ASan, UBSan, ASan+UBSan)
./run-all.py

# Run specific combinations
./run-all.py --asan --ubsan      # Just ASan and UBSan
./run-all.py --fuzz             # Just fuzzer (runs standalone-fuzzer.py)
./run-all.py --all              # Sanitizers plus fuzzer

# With parameters
./run-all.py --debug --runs 25 --enhanced
./run-all.py --build-only       # Just build everything
./run-all.py --test-only        # Just test (assume already built)
```

## Module Structure

- `lib/` - Shared modules
  - `colors.py` - Color output utilities
  - `build_lanes.py` - Build system management
  - `game_runner.py` - Hack 1.0.3 game automation
  - `test_runner.py` - Test execution and logging

- Individual runners:
  - `run-asan.py` - AddressSanitizer
  - `run-ubsan.py` - UndefinedBehaviorSanitizer  
  - `run-asan-ubsan.py` - Combined ASan+UBSan

- `standalone-fuzzer.py` - Standalone libFuzzer targets (self-contained, no `src/` linkage)
- `run-all.py` - Simple orchestration

## Key Improvements

1. **Modularity**: Each sanitizer can be run independently
2. **Focused Testing**: Target specific issues without running everything
3. **Standalone Fuzzer**: Self-contained libFuzzer targets (do not exercise `src/`)
4. **Clean Logs**: Timestamped logs in `test-logs/` and `fuzz-logs/`
5. **Better Error Handling**: Clear pass/fail status for each component
6. **UV Support**: Updated dependency messages for uv package manager

## Examples

```bash
# Quick ASan check (clean rebuild)
./run-asan.py --runs 10 --steps 20

# Deep UBSan analysis (clean rebuild)
./run-ubsan.py --runs 100 --steps 100 --debug --enhanced

# Full test suite with clean rebuilds
./run-all.py --runs 50 --fuzz-duration 300

# Fast iteration without clean rebuilds (development)
./run-all.py --no-clean --runs 25

# Just build everything for later testing
./run-all.py --build-only

# Development workflow: clean build once, fast test multiple times
./run-all.py --build-only                    # Clean build all
./run-asan.py --test-only --no-clean --debug # Fast test (no rebuild)
./run-ubsan.py --test-only --no-clean --enhanced # Fast test (no rebuild)
```

The original `gauntlet_v3.py` remains available but this modular approach provides better control and faster iteration.