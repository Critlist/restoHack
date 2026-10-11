#!/usr/bin/env python3
"""
Individual ASan+UBSan combined test runner for restoHack
"""

import os
import sys
import shutil
import argparse
from pathlib import Path
from datetime import datetime, timezone

# Put dev/ on sys.path so lib/ imports as a package (lib uses relative imports)
sys.path.insert(0, str(Path(__file__).resolve().parent))

from lib.colors import Colors
from lib.build_lanes import BuildLane, BuildManager
from lib.test_runner import TestRunner


def main():
    parser = argparse.ArgumentParser(description='restoHack ASan+UBSan Test Runner')
    parser.add_argument('--debug', action='store_true', help='Enable debug output')
    parser.add_argument('--runs', type=int, default=50, help='Number of test runs')
    parser.add_argument('--steps', type=int, default=40, help='Steps per test run')
    parser.add_argument('--enhanced', action='store_true', help='Enable enhanced testing')
    parser.add_argument('--build-only', action='store_true', help='Only build, don\'t test')
    parser.add_argument('--test-only', action='store_true', help='Only test (assume built)')
    parser.add_argument('--no-clean', action='store_true', help='Skip clean rebuild (faster but may have stale objects)')
    
    args = parser.parse_args()
    
    # Check requirements
    if not shutil.which('clang'):
        Colors.error("clang not found")
        return 1
    
    if not shutil.which('cmake'):
        Colors.error("cmake not found")
        return 1
        
    try:
        import pexpect
    except ImportError:
        Colors.error("pexpect module not found. Install with: uv add pexpect")
        return 1
    
    # Setup paths
    project_root = Path(__file__).parent.parent
    
    # Create timestamped log directory
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")
    log_dir = Path(__file__).parent / "test-logs" / f"asan-ubsan-{timestamp}"
    log_dir.mkdir(parents=True, exist_ok=True)
    
    # Create ASan+UBSan combined lane
    lane = BuildLane(
        "asan-ubsan",
        str(project_root / "build-asan-ubsan"),
        "-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all"
    )
    
    Colors.say("ASAN+UBSAN", f"🔥⚡ ASan+UBSan Combined Lane - Logs: {log_dir}")
    
    # Build phase
    if not args.test_only:
        build_manager = BuildManager(str(project_root), log_dir)
        clean_rebuild = not args.no_clean
        if not build_manager.build_lane(lane, clean_rebuild):
            Colors.error("ASan+UBSan build failed")
            return 1
        Colors.success("ASan+UBSan build completed")
    
    if args.build_only:
        Colors.success("Build-only mode complete")
        return 0
    
    # Test phase  
    test_runner = TestRunner(log_dir, args.debug, args.enhanced)
    passed, failed = test_runner.test_lane(lane, args.runs, args.steps)
    
    # Results
    total = passed + failed
    failure_rate = (failed * 100) // total if total > 0 else 0
    
    Colors.say("RESULTS", f"Passed: {passed}, Failed: {failed}, Rate: {failure_rate}%")
    
    if failure_rate > 10:  # Allow up to 10% failure for stress testing
        Colors.error(f"High failure rate: {failure_rate}%")
        return 1
    else:
        Colors.success("ASan+UBSan testing completed successfully")
        return 0


if __name__ == '__main__':
    sys.exit(main())