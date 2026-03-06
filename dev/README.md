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

# Enhanced fuzzer
./run-fuzz.py --duration 120
```

## Orchestration Script

Run multiple tests in sequence:

```bash
# Default: Run all sanitizers (ASan, UBSan, ASan+UBSan)
./run-all.py

# Run specific combinations
./run-all.py --asan --ubsan      # Just ASan and UBSan
./run-all.py --fuzz             # Just fuzzer
./run-all.py --all              # Everything including fuzzer

# With parameters
./run-all.py --debug --runs 25 --enhanced
./run-all.py --build-only       # Just build everything
./run-all.py --test-only        # Just test (assume already built)
```

## Module Structure

- `lib/` - Shared modules
  - `colors.py` - Color output utilities
  - `build_lanes.py` - Build system management
  - `game_runner.py` - 1984 Hack game automation
  - `test_runner.py` - Test execution and logging

- Individual runners:
  - `run-asan.py` - AddressSanitizer
  - `run-ubsan.py` - UndefinedBehaviorSanitizer  
  - `run-asan-ubsan.py` - Combined ASan+UBSan
  - `run-fuzz.py` - Enhanced libFuzzer

- `run-all.py` - Simple orchestration

## Key Improvements

1. **Modularity**: Each sanitizer can be run independently
2. **Focused Testing**: Target specific issues without running everything
3. **Enhanced Fuzzer**: Targets actual hack functions with proper corpus
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