#!/usr/bin/env python3
"""
Hardened-build test runner for restoHack

No sanitizers: a run is clean when the game exits 0 and no signal (e.g.
a _FORTIFY_SOURCE or stack-protector abort) killed it.
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
    parser = argparse.ArgumentParser(description='restoHack Hardened Test Runner')
    parser.add_argument('--debug', action='store_true', help='Enable debug output')
    parser.add_argument('--runs', type=int, default=50, help='Number of test runs')
    parser.add_argument('--steps', type=int, default=40, help='Steps per test run')
    parser.add_argument('--enhanced', action='store_true', help='Enable enhanced testing')
    parser.add_argument('--build-only', action='store_true', help='Only build, don\'t test')
    parser.add_argument('--test-only', action='store_true', help='Only test (assume built)')
    parser.add_argument('--no-clean', action='store_true', help='Skip clean rebuild (faster but may have stale objects)')
    
    parser.add_argument('--seed-offset', type=int, default=0,
                        help='Mixed into every run seed (default 0 keeps the standard seeds)')
    parser.add_argument('--only-run', type=int, default=0, metavar='N',
                        help='Replay just run N (use the same --seed-offset, --steps, --enhanced)')
    
    parser.add_argument('--tolerate-failure-percent', type=int, default=0,
                        help='Opt-in: allow up to this percent of failed runs (default 0: any failure fails)')
    
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
    log_dir = Path(__file__).parent / "test-logs" / f"hardened-{timestamp}"
    log_dir.mkdir(parents=True, exist_ok=True)
    
    # Hardened lane (flags defined once, in BuildManager.get_standard_lanes)
    lane = next(l for l in BuildManager(str(project_root), log_dir).get_standard_lanes()
                if l.name == "hardened")
    
    Colors.say("HARDENED", f"🛡️  Hardened Lane - Logs: {log_dir}")
    
    # Build phase
    if not args.test_only:
        build_manager = BuildManager(str(project_root), log_dir)
        clean_rebuild = not args.no_clean
        if not build_manager.build_lane(lane, clean_rebuild):
            Colors.error("Hardened build failed")
            return 1
        Colors.success("Hardened build completed")
    
    if args.build_only:
        Colors.success("Build-only mode complete")
        return 0
    
    # Test phase  
    test_runner = TestRunner(log_dir, args.debug, args.enhanced)
    passed, failed = test_runner.test_lane(lane, args.runs, args.steps,
                                           args.seed_offset, args.only_run)
    
    # Results
    total = passed + failed
    failure_rate = (failed * 100) // total if total > 0 else 0
    
    Colors.say("RESULTS", f"Passed: {passed}, Failed: {failed}, Rate: {failure_rate}%")
    
    if total == 0 or failed * 100 > args.tolerate_failure_percent * total:
        Colors.error(f"FAILED: {failed}/{total} runs failed ({failure_rate}%)")
        return 1
    else:
        Colors.success("Hardened testing completed successfully")
        return 0


if __name__ == '__main__':
    sys.exit(main())