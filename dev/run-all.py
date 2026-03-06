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

# Add lib directory to path for imports
sys.path.insert(0, str(Path(__file__).parent / 'lib'))

from colors import Colors


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
  %(prog)s                    # Run all sanitizers (ASan, UBSan, ASan+UBSan)
  %(prog)s --asan             # Run only ASan
  %(prog)s --ubsan            # Run only UBSan  
  %(prog)s --asan-ubsan       # Run only ASan+UBSan combined
  %(prog)s --fuzz             # Run only fuzzer
  %(prog)s --all              # Run sanitizers + fuzzer
  %(prog)s --debug --runs 25  # Debug mode with 25 runs per sanitizer
  %(prog)s --build-only       # Only build, don't test
        ''')
    
    # Individual test selection
    parser.add_argument('--asan', action='store_true', help='Run ASan only')
    parser.add_argument('--ubsan', action='store_true', help='Run UBSan only')
    parser.add_argument('--asan-ubsan', action='store_true', help='Run ASan+UBSan only')
    parser.add_argument('--fuzz', action='store_true', help='Run fuzzer only')
    parser.add_argument('--all', action='store_true', help='Run sanitizers + fuzzer')
    
    # Test parameters
    parser.add_argument('--debug', action='store_true', help='Enable debug output')
    parser.add_argument('--runs', type=int, default=50, help='Test runs per sanitizer')
    parser.add_argument('--steps', type=int, default=40, help='Steps per test run')
    parser.add_argument('--enhanced', action='store_true', help='Enhanced testing mode')
    parser.add_argument('--build-only', action='store_true', help='Only build, don\'t test')
    parser.add_argument('--test-only', action='store_true', help='Only test (assume built)')
    parser.add_argument('--no-clean', action='store_true', help='Skip clean rebuild (faster but may have stale objects)')
    
    # Fuzzer parameters
    parser.add_argument('--fuzz-duration', type=int, default=60, help='Fuzz duration per target (seconds)')
    
    args = parser.parse_args()
    
    # Determine what to run
    run_asan = args.asan or (not any([args.asan, args.ubsan, args.asan_ubsan, args.fuzz]))
    run_ubsan = args.ubsan or (not any([args.asan, args.ubsan, args.asan_ubsan, args.fuzz]))  
    run_asan_ubsan = args.asan_ubsan or (not any([args.asan, args.ubsan, args.asan_ubsan, args.fuzz]))
    run_fuzz = args.fuzz or args.all
    
    # If specific flags used, only run those
    if any([args.asan, args.ubsan, args.asan_ubsan, args.fuzz]):
        run_asan = args.asan
        run_ubsan = args.ubsan
        run_asan_ubsan = args.asan_ubsan
        run_fuzz = args.fuzz
    
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
    
    common_args.extend(['--runs', str(args.runs)])
    common_args.extend(['--steps', str(args.steps)])
    
    # Track results
    results = []
    start_time = datetime.now()
    
    Colors.say("START", f"🎯 restoHack Test Orchestrator")
    if args.build_only:
        Colors.say("MODE", "🔧 Build-only mode")
    elif args.test_only:
        Colors.say("MODE", "🧪 Test-only mode") 
    else:
        Colors.say("MODE", f"🚀 Full test mode: {args.runs} runs × {args.steps} steps")
    
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
    
    # Run fuzzer
    if run_fuzz:
        fuzz_args = ['--duration', str(args.fuzz_duration)]
        if args.debug:
            fuzz_args.append('--debug')
        success = run_script("standalone-fuzzer.py", fuzz_args, "libFuzzer")
        results.append(("Fuzzer", success))
    
    # Summary
    elapsed = datetime.now() - start_time
    Colors.say("COMPLETE", f"⏱️  Total time: {elapsed}")
    
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