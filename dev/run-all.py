#!/usr/bin/env python3
"""
Simplified orchestration script for restoHack testing
Run all sanitizers in sequence or individually
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path
from datetime import datetime, timezone

# Put dev/ on sys.path so lib/ imports as a package (lib uses relative imports)
sys.path.insert(0, str(Path(__file__).resolve().parent))

from lib.colors import Colors


def run_script(script_name: str, args: list, description: str) -> bool:
    """Run a test script and return success status"""
    script_path = Path(__file__).parent / script_name
    if not script_path.exists():
        Colors.error(f"Script not found: {script_name}")
        return False
    
    Colors.say("RUN", f"🚀 {description}")
    
    try:
        cmd = [sys.executable, str(script_path)] + args
        result = subprocess.run(cmd, check=False)
        
        if result.returncode == 0:
            Colors.success(f"✅ {description} - PASSED")
            return True
        else:
            Colors.error(f"❌ {description} - FAILED (exit code: {result.returncode})")
            return False
            
    except KeyboardInterrupt:
        Colors.warn("⚠️  Interrupted by user")
        return False
    except Exception as e:
        Colors.error(f"❌ {description} - EXCEPTION: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(
        description='restoHack Test Orchestrator - Run all sanitizers or individual tests',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s                    # Run all lanes (ASan, UBSan, ASan+UBSan, hardened)
  %(prog)s --asan             # Run only ASan
  %(prog)s --ubsan            # Run only UBSan  
  %(prog)s --asan-ubsan       # Run only ASan+UBSan combined
  %(prog)s --hardened         # Run only the hardened (no-sanitizer) lane
  %(prog)s --fuzz             # Run only fuzzer
  %(prog)s --all              # Run all lanes + fuzzer
  %(prog)s --debug --runs 25  # Debug mode with 25 runs per sanitizer
  %(prog)s --build-only       # Only build, don't test
  %(prog)s --runs 300 --steps 1000 --enhanced --seed-offset $(date +%%s)   # Soak
        ''')
    
    # Individual test selection
    parser.add_argument('--asan', action='store_true', help='Run ASan only')
    parser.add_argument('--ubsan', action='store_true', help='Run UBSan only')
    parser.add_argument('--asan-ubsan', action='store_true', help='Run ASan+UBSan only')
    parser.add_argument('--hardened', action='store_true', help='Run hardened lane only')
    parser.add_argument('--fuzz', action='store_true', help='Run fuzzer only')
    parser.add_argument('--all', action='store_true', help='Run all lanes + fuzzer')
    
    # Test parameters
    parser.add_argument('--debug', action='store_true', help='Enable debug output')
    parser.add_argument('--runs', type=int, default=50, help='Test runs per sanitizer')
    parser.add_argument('--steps', type=int, default=40, help='Steps per test run')
    parser.add_argument('--enhanced', action='store_true', help='Enhanced testing mode')
    parser.add_argument('--build-only', action='store_true', help='Only build, don\'t test')
    parser.add_argument('--test-only', action='store_true', help='Only test (assume built)')
    parser.add_argument('--no-clean', action='store_true', help='Skip clean rebuild (faster but may have stale objects)')
    parser.add_argument('--seed-offset', type=int, default=0,
                        help='Mixed into every run seed (default 0 keeps the standard seeds)')
    parser.add_argument('--only-run', type=int, default=0, metavar='N',
                        help='Replay just run N in each selected lane')
    
    parser.add_argument('--tolerate-failure-percent', type=int, default=0,
                        help='Opt-in: allow up to this percent of failed runs per sanitizer (default 0)')
    
    # Fuzzer parameters
    parser.add_argument('--fuzz-duration', type=int, default=60, help='Fuzz duration per target (seconds)')
    
    args = parser.parse_args()
    
    # Determine what to run: specific flags select only those; otherwise
    # every lane (plus the fuzzer with --all)
    selected = any([args.asan, args.ubsan, args.asan_ubsan, args.hardened, args.fuzz])
    run_asan = args.asan if selected else True
    run_ubsan = args.ubsan if selected else True
    run_asan_ubsan = args.asan_ubsan if selected else True
    run_hardened = args.hardened if selected else True
    run_fuzz = args.fuzz if selected else args.all
    
    # Build common arguments for sanitizer tests
    common_args = []
    if args.debug:
        common_args.append('--debug')
    if args.enhanced:
        common_args.append('--enhanced')
    if args.build_only:
        common_args.append('--build-only')
    if args.test_only:
        common_args.append('--test-only')
    if args.no_clean:
        common_args.append('--no-clean')
    
    common_args.extend(['--tolerate-failure-percent', str(args.tolerate_failure_percent)])
    common_args.extend(['--runs', str(args.runs)])
    common_args.extend(['--steps', str(args.steps)])
    common_args.extend(['--seed-offset', str(args.seed_offset)])
    if args.only_run:
        common_args.extend(['--only-run', str(args.only_run)])
    
    # Track results
    results = []
    start_time = datetime.now()
    
    Colors.say("START", f"🎯 restoHack Test Orchestrator")
    if args.build_only:
        Colors.say("MODE", "🔧 Build-only mode")
    elif args.test_only:
        Colors.say("MODE", "🧪 Test-only mode") 
    else:
        Colors.say("MODE", f"🚀 Full test mode: {args.runs} runs × {args.steps} steps, "
                           f"seed offset {args.seed_offset}")
    
    # Run sanitizers
    if run_asan:
        success = run_script("run-asan.py", common_args, "AddressSanitizer (ASan)")
        results.append(("ASan", success))
    
    if run_ubsan:
        success = run_script("run-ubsan.py", common_args, "UndefinedBehaviorSanitizer (UBSan)")
        results.append(("UBSan", success))
        
    if run_asan_ubsan:
        success = run_script("run-asan-ubsan.py", common_args, "ASan+UBSan Combined")
        results.append(("ASan+UBSan", success))

    if run_hardened:
        success = run_script("run-hardened.py", common_args, "Hardened (no sanitizers)")
        results.append(("Hardened", success))
    
    # Run fuzzer
    if run_fuzz:
        fuzz_args = ['--duration', str(args.fuzz_duration)]
        if args.debug:
            fuzz_args.append('--debug')
        success = run_script("standalone-fuzzer.py", fuzz_args, "libFuzzer")
        results.append(("Fuzzer", success))
    
    # Summary
    elapsed = datetime.now() - start_time
    Colors.say("COMPLETE", f"⏱️  Total time: {elapsed} (seed offset {args.seed_offset})")
    
    print()
    Colors.say("SUMMARY", "📊 Test Results:")
    
    passed = 0
    failed = 0
    
    for test_name, success in results:
        if success:
            print(f"  ✅ {test_name}: PASSED")
            passed += 1
        else:
            print(f"  ❌ {test_name}: FAILED") 
            failed += 1
    
    print()
    total = passed + failed
    if total > 0:
        success_rate = (passed * 100) // total
        Colors.say("RESULTS", f"Passed: {passed}/{total} ({success_rate}%)")
        
        if failed == 0:
            Colors.success("🎉 ALL TESTS PASSED - Code is battle-ready!")
            return 0
        else:
            Colors.error(f"💀 {failed} TESTS FAILED - Investigation required!")
            return 1
    else:
        Colors.warn("No tests were run")
        return 1


if __name__ == '__main__':
    sys.exit(main())