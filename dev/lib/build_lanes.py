#!/usr/bin/env python3
"""
Build lane configuration and management for restoHack testing
"""

import os
import subprocess
import shutil
from pathlib import Path
from typing import Dict, List, Tuple, Optional

from .colors import Colors


class BuildLane:
    """Represents a sanitizer build configuration"""
    
    def __init__(self, name: str, build_dir: str, cflags: str, env_vars: Optional[Dict[str, str]] = None):
        self.name = name
        self.build_dir = build_dir
        self.cflags = cflags
        self.env_vars = env_vars or {}
        self.binary_path = os.path.join(build_dir, 'hack')
        
    def __str__(self):
        return f"BuildLane({self.name}, {self.build_dir})"


class BuildManager:
    """Manages building different sanitizer configurations"""
    
    def __init__(self, project_root: str, log_dir: Path):
        self.project_root = Path(project_root)
        self.log_dir = log_dir
        
    def get_standard_lanes(self) -> List[BuildLane]:
        """Get standard set of build lanes"""
        return [
            BuildLane(
                "asan", 
                str(self.project_root / "build-asan"),
                "-fsanitize=address -fno-omit-frame-pointer -fno-sanitize-recover=all"
            ),
            BuildLane(
                "ubsan",
                str(self.project_root / "build-ubsan"), 
                "-fsanitize=undefined,bounds,shift,integer-divide-by-zero,signed-integer-overflow,null,unreachable,vla-bound,object-size -fno-omit-frame-pointer -fno-sanitize-recover=all"
            ),
            BuildLane(
                "asan-ubsan",
                str(self.project_root / "build-asan-ubsan"),
                "-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all"
            ),
            BuildLane(
                "hardened",
                str(self.project_root / "build-hardened"),
                "-O2 -fPIE -fstack-protector-strong -D_FORTIFY_SOURCE=2"
            )
        ]
        
    def run_command(self, cmd: List[str], cwd: Optional[str] = None, env: Optional[Dict[str, str]] = None) -> Tuple[int, str, str]:
        """Run command and capture output"""
        full_env = os.environ.copy()
        if env:
            full_env.update(env)
            
        try:
            result = subprocess.run(
                cmd, 
                cwd=cwd or str(self.project_root),
                env=full_env,
                capture_output=True,
                text=True,
                timeout=300  # 5 minute timeout
            )
            return result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired:
            return -1, "", "Command timed out"
        except Exception as e:
            return -1, "", str(e)
            
    def build_lane(self, lane: BuildLane, clean_rebuild: bool = True) -> bool:
        """Build a specific sanitizer lane with optional clean rebuild"""
        Colors.say("BUILD", f"{lane.name} -> {lane.build_dir} (clean={clean_rebuild})")
        
        # Clean build directory if requested
        if clean_rebuild and os.path.exists(lane.build_dir):
            shutil.rmtree(lane.build_dir)
            
        # Configure with proper build type and flags
        # Use project-supported build types: Debug, Release, Hardened
        if "hardened" in lane.name.lower():
            build_type = "Hardened"
        elif any(flag in lane.cflags for flag in ["-fsanitize", "-g"]):
            build_type = "Debug"
        else:
            build_type = "Release"
        # Test lanes only: lets HACK_SEED seed the game's RNG so runs replay
        cflags = f"{lane.cflags} -DHACK_TEST_SEED"
        cmake_cmd = [
            'cmake', '-S', str(self.project_root), '-B', lane.build_dir,
            f'-DCMAKE_BUILD_TYPE={build_type}',
            '-DCMAKE_C_COMPILER=clang',
            f'-DCMAKE_C_FLAGS_DEBUG={cflags}' if build_type == "Debug" else f'-DCMAKE_C_FLAGS_RELWITHDEBINFO={cflags}',
            '-DENABLE_SANITIZERS=ON' if 'sanitize' in lane.cflags else '-DENABLE_SANITIZERS=OFF'
        ]
        
        ret_code, stdout, stderr = self.run_command(cmake_cmd)
        
        # Log configure output
        config_log = self.log_dir / f"{lane.name}_configure.log"
        with open(config_log, 'w') as f:
            f.write(f"Command: {' '.join(cmake_cmd)}\n")
            f.write(f"Return code: {ret_code}\n")
            f.write(f"STDOUT:\n{stdout}\n")
            f.write(f"STDERR:\n{stderr}\n")
            
        if ret_code != 0:
            Colors.error(f"{lane.name} configure failed")
            return False
            
        # Build with clean-first flag for complete rebuild
        build_flags = ['--clean-first'] if clean_rebuild else []
        build_cmd = ['cmake', '--build', lane.build_dir, '-j'] + build_flags
        ret_code, stdout, stderr = self.run_command(build_cmd)
        
        # Log build output
        build_log = self.log_dir / f"{lane.name}_build.log"
        with open(build_log, 'w') as f:
            f.write(f"Command: {' '.join(build_cmd)}\n")
            f.write(f"Return code: {ret_code}\n")
            f.write(f"STDOUT:\n{stdout}\n")
            f.write(f"STDERR:\n{stderr}\n")
            
        if ret_code != 0:
            Colors.error(f"{lane.name} build failed")
            return False
            
        Colors.success(f"{lane.name} build completed")
        return True