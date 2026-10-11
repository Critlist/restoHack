#!/usr/bin/env python3
"""
Test runner for sanitizer lanes
"""

import os
import sys  
import time
import re
import shutil
import zlib
import tempfile
from pathlib import Path
from typing import Tuple, Dict, List

import pexpect

from .colors import Colors
from .game_runner import HackGameRunner
from .build_lanes import BuildLane


# dev/lib/test_runner.py -> repo root
REPO_ROOT = Path(__file__).resolve().parents[2]
# Level files the game writes into its hackdir: <name>.<level-number>
_LEVEL_FILE_RE = re.compile(r"^.+\.\d+$")


class TestRunner:
    """Runs tests for individual sanitizer lanes"""
    
    def __init__(self, log_dir: Path, debug: bool = False, enhanced_mode: bool = False):
        # Absolute: the game chdir()s into its hackdir, so a relative
        # sanitizer log_path would silently point somewhere else.
        self.log_dir = Path(log_dir).resolve()
        self.debug = debug
        self.enhanced_mode = enhanced_mode
        self._setup_core_dumps()
        
    def _setup_core_dumps(self) -> None:
        """Enable core dumps for crash analysis"""
        try:
            import resource
            resource.setrlimit(resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.RLIM_INFINITY))
            Colors.say("SETUP", "Core dumps enabled")
        except Exception as e:
            Colors.error(f"Failed to enable core dumps: {e}")
    
    @staticmethod
    def run_seed(lane_name: str, run_number: int) -> int:
        """Stable per-(lane, run) seed (Python's hash() is randomized per process)."""
        return zlib.crc32(f"{lane_name}_{run_number}".encode())

    def _lane_env(self, lane: BuildLane, run_number: int) -> Dict[str, str]:
        """Generate deterministic environment for each lane"""
        env = os.environ.copy()
        logbase = str(self.log_dir / f"{lane.name}_run{run_number:03d}")
        sym = shutil.which("llvm-symbolizer") or shutil.which("addr2line") or ""
        
        # Set symbolizer for sanitizers
        if sym:
            env["ASAN_SYMBOLIZER_PATH"] = sym
            env["UBSAN_SYMBOLIZER_PATH"] = sym
        
        # Sanitizer-specific options with logging
        if "asan" in lane.name:
            env["ASAN_OPTIONS"] = (
                f"abort_on_error=1:strict_string_checks=1:detect_stack_use_after_return=1:"
                f"check_initialization_order=1:detect_leaks=1:log_path={logbase}.asan"
            )
        if "ubsan" in lane.name:
            env["UBSAN_OPTIONS"] = f"print_stacktrace=1:halt_on_error=1:log_path={logbase}.ubsan"
        
        # Add any lane-specific env vars
        env.update(lane.env_vars)
        
        # Informational only: the game does not read HACK_SEED.  The harness's
        # own choices are seeded in HackGameRunner via run_seed().
        env["HACK_SEED"] = str(self.run_seed(lane.name, run_number))
        
        return env
        
    def _clean_lane_artifacts(self, lane: BuildLane) -> None:
        """Remove stale game state from this lane's own hackdir.

        Deletes only what the game itself writes: hackdir/save/*, level files
        (<name>.<N>) and game.lock.  Never touches anything outside
        <lane.build_dir>/hackdir, and refuses to run if that directory is not
        inside the repository checkout this harness lives in.
        """
        hackdir = (Path(lane.build_dir) / "hackdir").resolve()
        try:
            hackdir.relative_to(REPO_ROOT)
        except ValueError:
            Colors.error(f"Refusing to clean {hackdir}: outside {REPO_ROOT}")
            return
        if not hackdir.is_dir():
            return

        victims = []
        save_dir = hackdir / "save"
        if save_dir.is_dir():
            victims.extend(p for p in save_dir.iterdir() if p.is_file())
        victims.extend(p for p in hackdir.iterdir()
                       if p.is_file() and _LEVEL_FILE_RE.match(p.name))
        lock = hackdir / "game.lock"
        if lock.is_file():
            victims.append(lock)

        for victim in victims:
            try:
                victim.unlink()
                if self.debug:
                    print(f"[CLEAN] removed {victim}", file=sys.stderr)
            except FileNotFoundError:
                pass
            except OSError as e:
                Colors.error(f"Could not remove {victim}: {e}")

    def test_lane(self, lane: BuildLane, runs: int = 50, steps: int = 40) -> Tuple[int, int]:
        """Test a built lane with stress runs and proper logging"""
        if not os.path.exists(lane.binary_path):
            Colors.error(f"Binary not found: {lane.binary_path}")
            return 0, runs
            
        Colors.say("TEST", f"{lane.name} stress: {runs} runs x {steps} steps")
        
        passed = 0
        failed = 0
        
        for i in range(1, runs + 1):
            run_env = self._lane_env(lane, i)
            seed = self.run_seed(lane.name, i)
            
            try:
                self._clean_lane_artifacts(lane)
                
                # Use corrected pexpect approach with proper game flow
                with tempfile.NamedTemporaryFile(mode="w+b", delete=False) as temp_stderr:
                    runner = HackGameRunner(lane.binary_path, debug=self.debug, enhanced_mode=self.enhanced_mode,
                                            seed=self.run_seed(lane.name, i))
                    
                    # Start the process with stderr redirection
                    # Convert to proper environment format
                    env_dict = dict(os.environ)
                    env_dict.update(run_env)
                    child = pexpect.spawn(f'{lane.binary_path} -D', timeout=10, env=env_dict)
                    child.logfile_read = temp_stderr
                    runner.child = child
                    
                    # Run the game session with proper interaction
                    player_name = f"Player{i:02d}"
                    success, exit_code = runner.run_session(steps, player_name)
                
                # Read stderr for sanitizer errors
                stderr_output = ""
                try:
                    with open(temp_stderr.name, "r") as f:
                        stderr_output = f.read()
                    os.unlink(temp_stderr.name)  # Clean up temp file
                except:
                    pass
                
                # Check for actual memory/safety errors (not player death)
                has_asan_errors = "AddressSanitizer" in stderr_output
                has_ubsan_errors = ("runtime error:" in stderr_output) or ("SUMMARY: UndefinedBehaviorSanitizer" in stderr_output)
                has_sanitizer_errors = has_asan_errors or has_ubsan_errors
                
                # Check if process was killed by actual signal (segfault, abort, etc.)
                # Don't treat pexpect timeouts/terminations as signals
                killed_by_signal = False
                if hasattr(runner, 'child') and runner.child:
                    # Only treat as signal if pexpect detected an actual signal
                    killed_by_signal = (runner.child.signalstatus is not None and 
                                       runner.child.signalstatus != 0)
                
                # Sanitizer reports written to log_path files (e.g. LeakSanitizer
                # at exit) never appear on the pty stream, so check them too.
                log_findings = self._check_sanitizer_logs(lane.name, i)
                has_log_errors = bool(log_findings)

                # Only write logs for actual memory/safety errors
                if has_sanitizer_errors or killed_by_signal or has_log_errors:
                    stderr_log_path = self.log_dir / f"{lane.name}_run{i:03d}.stderr"
                    run_log_path = self.log_dir / f"{lane.name}_run{i:03d}.log"
                    
                    # Write stderr file
                    with open(stderr_log_path, "w") as f:
                        f.write(stderr_output)
                    
                    # Write combined log
                    with open(run_log_path, "w") as log_file:
                        log_file.write(f"Exit code: {exit_code}\n")
                        log_file.write(f"Success: {success}\n")
                        log_file.write(f"Seed: {seed} (lane={lane.name} run={i})\n")
                        if killed_by_signal:
                            log_file.write(f"Signal: Process killed by signal\n")
                        for label, path in log_findings:
                            log_file.write(f"Sanitizer log ({label}): {path}\n")
                        log_file.write(f"STDERR:\n{stderr_output}\n")
                
                # PASS: No sanitizer errors and no signals = healthy run
                # Player death and normal quit are acceptable; a hung quit is not
                is_healthy = (success and not has_sanitizer_errors and not killed_by_signal
                              and not has_log_errors)
                
                if is_healthy:
                    print(f"✅ {lane.name} {i:03d}")
                    passed += 1
                else:
                    error_reason = []
                    if has_asan_errors: error_reason.append("asan-error")
                    if has_ubsan_errors: error_reason.append("ubsan-error") 
                    for label, _path in log_findings:
                        if label not in error_reason:
                            error_reason.append(label)
                    if killed_by_signal: error_reason.append("signal")
                    if not success: error_reason.append(f"session-failed: {runner.fail_reason or 'unknown'}")
                    print(f"❌ {lane.name} {i:03d} FAILED ({', '.join(error_reason)}) seed={seed}")
                    failed += 1
                        
            except Exception as e:
                print(f"❌ {lane.name} {i:03d} EXCEPTION: {e} seed={seed}")
                failed += 1
        
        return passed, failed
    
    @staticmethod
    def _classify_sanitizer_log(path: Path) -> str:
        """Label a sanitizer log file by its content.

        LeakSanitizer reports go to the ASan log (log_path=<base>.asan.<pid>),
        so the file name alone cannot distinguish leaks from other ASan errors.
        """
        try:
            text = path.read_text(errors="replace")
        except OSError as e:
            return f"unreadable-log({e.__class__.__name__})"
        if "LeakSanitizer" in text or "leaked in" in text:
            return "leak"
        if ".ubsan." in path.name or "UndefinedBehaviorSanitizer" in text or "runtime error:" in text:
            return "ubsan"
        return "asan"

    def _check_sanitizer_logs(self, lane_name: str, run_number: int) -> List[Tuple[str, Path]]:
        """Find sanitizer report files for a run.

        Sanitizers write log_path=<log_dir>/<lane>_run<NNN>.asan|.ubsan and
        append .<pid>, hence the trailing '.*'.  Returns (label, path) pairs;
        an empty list means no reports.
        """
        findings: List[Tuple[str, Path]] = []
        for kind in ("asan", "ubsan"):
            for log in sorted(self.log_dir.glob(f"{lane_name}_run{run_number:03d}.{kind}.*")):
                label = self._classify_sanitizer_log(log)
                Colors.error(f"Sanitizer report ({label}) in {log.name}")
                findings.append((label, log))
        return findings
