#!/usr/bin/env python3
"""
Valgrind memcheck lane for restoHack

Runs the same pexpect-driven sessions as the sanitizer lanes, but under
valgrind memcheck. A run fails on any memcheck error or on definite,
indirect or possible leaks; still-reachable blocks (ncurses/terminfo) are
expected and ignored, as are system-library leaks covered by valgrind.supp.
"""

import argparse
import os
import re
import shutil
import sys
from datetime import datetime, timezone
from pathlib import Path

# Put dev/ on sys.path so lib/ imports as a package (lib uses relative imports)
sys.path.insert(0, str(Path(__file__).resolve().parent))

from lib.colors import Colors
from lib.build_lanes import BuildLane, BuildManager
from lib.game_runner import HackGameRunner
from lib.test_runner import TestRunner

DEV_DIR = Path(__file__).resolve().parent
SUPPRESSIONS = DEV_DIR / "valgrind.supp"

# Memcheck settings. --error-exitcode makes a failing run visible through the
# game's exit status as well, which the summary below double-checks.
VALGRIND_FLAGS = [
    "--tool=memcheck",
    "--track-origins=yes",
    "--leak-check=full",
    "--show-leak-kinds=definite,indirect",
    "--errors-for-leak-kinds=definite,indirect",
    "--track-fds=yes",
    "--num-callers=25",
    "--error-exitcode=99",
]

# Memcheck runs the game 20-50x slower, so the harness's default quiet period
# between keys is too short: it would treat a slow-but-live game as quiet and
# then pile up keys behind a --More-- prompt.
VALGRIND_SETTLE = 0.25


class ValgrindGameRunner(HackGameRunner):
    """HackGameRunner with a settle window suited to memcheck's slowdown."""

    def _drain(self, settle: float = VALGRIND_SETTLE) -> None:
        super()._drain(settle)


ERROR_SUMMARY_RE = re.compile(r"ERROR SUMMARY: (\d+) errors")
LOST_RE = re.compile(
    r"(definitely|indirectly|possibly) lost: ([\d,]+) bytes in ([\d,]+) blocks")


def make_command(binary: str, log_file: Path) -> str:
    """Valgrind invocation for one session, as a pexpect command string."""
    parts = ["valgrind", *VALGRIND_FLAGS]
    if SUPPRESSIONS.is_file():
        parts.append(f"--suppressions={SUPPRESSIONS}")
    parts.append(f"--log-file={log_file}")
    parts.append(binary)
    return " ".join(parts)


def read_report(log_file: Path):
    """Return (errors, leaks, fd_report) from a memcheck log."""
    try:
        text = log_file.read_text(errors="replace")
    except OSError:
        return None, None, None

    m = ERROR_SUMMARY_RE.search(text)
    errors = int(m.group(1)) if m else None

    leaks = {}
    for kind, nbytes, blocks in LOST_RE.findall(text):
        leaks[kind] = int(nbytes.replace(",", ""))
    leaked = sum(leaks.values()) if leaks else None

    fds = re.search(r"FILE DESCRIPTORS: (\d+) open", text)
    fd_report = int(fds.group(1)) if fds else None
    return errors, leaked, fd_report


def main():
    parser = argparse.ArgumentParser(description='restoHack Valgrind memcheck lane')
    parser.add_argument('--debug', action='store_true', help='Enable debug output')
    parser.add_argument('--runs', type=int, default=10, help='Number of test runs')
    parser.add_argument('--steps', type=int, default=100, help='Steps per test run')
    parser.add_argument('--enhanced', action='store_true',
                        help='Enable enhanced testing (save/resume, inventory, resize)')
    parser.add_argument('--build-only', action='store_true', help="Only build, don't test")
    parser.add_argument('--test-only', action='store_true', help='Only test (assume built)')
    parser.add_argument('--no-clean', action='store_true', help='Skip clean rebuild')

    parser.add_argument('--seed-offset', type=int, default=0,
                        help='Mixed into every run seed (default 0 keeps the standard seeds)')
    parser.add_argument('--only-run', type=int, default=0, metavar='N',
                        help='Replay just run N (use the same --seed-offset, --steps, --enhanced)')
    parser.add_argument('--tolerate-failure-percent', type=int, default=0,
                        help='Opt-in: allow up to this percent of failed runs '
                             '(default 0: any failure fails)')

    args = parser.parse_args()

    for tool in ('valgrind', 'clang', 'cmake'):
        if not shutil.which(tool):
            Colors.error(f"{tool} not found")
            return 1

    try:
        import pexpect
    except ImportError:
        Colors.error("pexpect module not found. Install with: uv add pexpect")
        return 1

    project_root = Path(__file__).parent.parent

    timestamp = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")
    log_dir = DEV_DIR / "test-logs" / f"valgrind-{timestamp}"
    log_dir.mkdir(parents=True, exist_ok=True)

    # Debug build: no optimisations, full symbols for readable stacks.
    lane = BuildLane("valgrind", str(project_root / "build-valgrind"), "-g")

    Colors.say("VALGRIND", f"🧠 Memcheck Lane - Logs: {log_dir}")

    if not args.test_only:
        build_manager = BuildManager(str(project_root), log_dir)
        if not build_manager.build_lane(lane, not args.no_clean):
            Colors.error("Valgrind build failed")
            return 1
        Colors.success("Valgrind build completed")

    if args.build_only:
        return 0

    runner = TestRunner(log_dir, debug=args.debug, enhanced_mode=args.enhanced)
    run_numbers = [args.only_run] if args.only_run else range(1, args.runs + 1)

    Colors.say("TEST", f"valgrind stress: {len(run_numbers)} runs x {args.steps} steps"
                       f" (seed offset {args.seed_offset})")

    passed = failed = 0
    for i in run_numbers:
        seed = runner.run_seed(lane.name, i, args.seed_offset)
        env = runner._lane_env(lane, i, seed)
        log_file = log_dir / f"valgrind_run{i:03d}.%p.log"
        command = make_command(lane.binary_path, log_file)

        try:
            runner._clean_lane_artifacts(lane)
            child = pexpect.spawn(f"{command} -D", timeout=600, env=env,
                                  dimensions=(24, 80))
            game = ValgrindGameRunner(lane.binary_path, debug=args.debug,
                                      enhanced_mode=args.enhanced, seed=seed)
            game.child = child
            success, exit_code = game.run_session(args.steps, f"Player{i:02d}")
            try:
                child.close(force=True)
            except Exception:
                pass

            reports = sorted(log_dir.glob(f"valgrind_run{i:03d}.*.log"))
            if not reports:
                print(f"❌ {lane.name} {i:03d} FAILED (no valgrind log; "
                      f"session={success} exit={exit_code})")
                failed += 1
                continue

            errors, leaked, fds = read_report(reports[0])
            problems = []
            if errors is None:
                problems.append("unreadable-log")
            elif errors:
                problems.append(f"{errors} memcheck error(s)")
            if leaked is None:
                problems.append("no-leak-summary")
            elif leaked:
                problems.append(f"{leaked} bytes leaked")
            if not success:
                problems.append(f"session-failed: {game.fail_reason or 'unknown'}")
            if exit_code not in (0,):
                problems.append(f"exit {exit_code}")

            if problems:
                print(f"❌ {lane.name} {i:03d} FAILED ({', '.join(problems)}) "
                      f"seed={seed} log={reports[0]}")
                failed += 1
            else:
                extra = f" (fds {fds})" if fds is not None else ""
                print(f"✅ {lane.name} {i:03d}{extra}")
                passed += 1
        except Exception as e:
            print(f"❌ {lane.name} {i:03d} EXCEPTION: {e} seed={seed}")
            failed += 1

    total = passed + failed
    failure_rate = (failed * 100) // total if total else 0
    Colors.say("RESULTS", f"Passed: {passed}, Failed: {failed}, Rate: {failure_rate}%")
    if total == 0 or failed * 100 > args.tolerate_failure_percent * total:
        Colors.error("Valgrind memcheck found problems")
        return 1
    Colors.success("Valgrind memcheck completed successfully")
    return 0


if __name__ == '__main__':
    sys.exit(main())
