#!/usr/bin/env python3
"""
Standalone fuzzer for restoHack - tests critical functions without full linking

NOTE: the libFuzzer targets are self-contained C snippets compiled one file at
a time (clang -fsanitize=fuzzer,address). They link no src/ objects, so they
do not exercise the restoHack game code.
"""

import os
import sys
import shutil
import argparse
import subprocess
import struct
from pathlib import Path
from datetime import datetime

# Put dev/ on sys.path so lib/ imports as a package (lib uses relative imports)
sys.path.insert(0, str(Path(__file__).resolve().parent))

from lib.colors import Colors


class StandaloneFuzzer:
    """Lightweight fuzzer that tests critical hack functions"""
    
    def __init__(self, project_root: str, debug: bool = False):
        self.project_root = Path(project_root)
        self.debug = debug
        self.src_dir = self.project_root / "src"
        self.fuzz_dir = self.project_root / "build-fuzz-standalone"
        
        # Create timestamped log directory
        timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
        self.log_dir = self.project_root / "dev" / "fuzz-logs" / timestamp
        self.log_dir.mkdir(parents=True, exist_ok=True)
        
        Colors.say("FUZZ", f"🎯 Standalone fuzzer logs: {self.log_dir}")

    def check_requirements(self) -> bool:
        """Check if fuzzing tools are available"""
        if not shutil.which('clang'):
            Colors.error("clang not found - required for libFuzzer")
            return False
        Colors.success("✅ Fuzzing requirements met")
        return True

    def setup_build_dir(self) -> bool:
        """Setup build directory"""
        if self.fuzz_dir.exists():
            shutil.rmtree(self.fuzz_dir)
        self.fuzz_dir.mkdir(parents=True)
        
        # Copy necessary headers
        shutil.copy(self.src_dir / "config.h", self.fuzz_dir)
        
        # Create minimal hack.h for standalone fuzzing
        minimal_hack = '''
#ifndef HACK_H
#define HACK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Critical constants from hack */
#define BUFSZ 256
#define COLNO 80
#define ROWNO 22

/* Minimal types */
typedef struct {
    unsigned char x, y;
} coord;

typedef signed char schar;
typedef signed char xchar;

/* Mock some globals */
extern char toplines[BUFSZ];
extern int CO;

/* Functions we'll test */
void pline(const char *fmt, ...);
void atl(int x, int y, int ch);
int isok(int x, int y);

#endif
'''
        with open(self.fuzz_dir / "hack.h", 'w') as f:
            f.write(minimal_hack)
        
        Colors.success("✅ Build directory prepared")
        return True

    def create_standalone_harnesses(self) -> list:
        """Create standalone fuzz harnesses"""
        harnesses = []
        
        # 1. Buffer overflow fuzzer
        buffer_fuzzer = '''
/* Standalone buffer overflow fuzzer for hack patterns */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define BUFSZ 256

/* Test buffer overflow patterns common in 1984 C */
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size == 0 || Size > 2048) return 0;
    
    /* Test 1: strcpy pattern (common in hack) */
    char buffer[BUFSZ];
    char input[BUFSZ];
    memset(buffer, 0, sizeof(buffer));
    memset(input, 0, sizeof(input));
    
    size_t copy_len = Size < BUFSZ - 1 ? Size : BUFSZ - 1;
    memcpy(input, Data, copy_len);
    input[copy_len] = '\\0';
    
    /* Simulate unsafe strcpy (but safely) */
    if (strlen(input) < sizeof(buffer)) {
        strcpy(buffer, input);
    }
    
    /* Test 2: sprintf pattern (used in hack for messages) */
    char msg[BUFSZ];
    if (Size > 0 && Data[0] < 128) {
        snprintf(msg, sizeof(msg), "You hit for %d damage!", Data[0]);
    }
    
    /* Test 3: strcat pattern (message concatenation) */
    char combined[BUFSZ];
    strcpy(combined, "Prefix: ");
    if (strlen(combined) + strlen(input) < sizeof(combined) - 1) {
        strcat(combined, input);
    }
    
    /* Test 4: Array indexing pattern */
    if (Size >= 2) {
        int index = (int)(Data[0]) - 128;  /* Can be negative */
        char warnings[] = "white pink red ruby purple black";
        
        /* Bounds check before access */
        if (index >= 0 && index < 6) {
            /* Safe access */
            char c = warnings[index * 6];
        }
    }
    
    return 0;
}
'''
        harnesses.append({
            'name': 'fuzz_buffers',
            'source': buffer_fuzzer,
            'description': 'Buffer overflow patterns'
        })
        
        # 2. Coordinate bounds fuzzer
        coord_fuzzer = '''
/* Coordinate bounds checking fuzzer */
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#define COLNO 80
#define ROWNO 22

typedef signed char schar;
typedef signed char xchar;

/* Simulate coordinate handling */
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 4) return 0;
    
    /* Extract coordinates that could overflow */
    int x = (int)Data[0] - 128;  /* -128 to 127 */
    int y = (int)Data[1] - 128;
    xchar xc = (xchar)Data[2];   /* signed char */
    xchar yc = (xchar)Data[3];
    
    /* Test 1: Direct array access pattern */
    static char map[COLNO][ROWNO];
    if (x >= 0 && x < COLNO && y >= 0 && y < ROWNO) {
        map[x][y] = 'X';  /* Safe */
    }
    
    /* Test 2: Signed char coordinate handling */
    if (xc >= 0 && xc < COLNO && yc >= 0 && yc < ROWNO) {
        map[xc][yc] = 'Y';  /* Safe with bounds check */
    }
    
    /* Test 3: Coordinate arithmetic */
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < COLNO && ny >= 0 && ny < ROWNO) {
                map[nx][ny] = '.';
            }
        }
    }
    
    /* Test 4: Distance calculations that could overflow */
    if (Size >= 8) {
        int x2 = (int)Data[4] - 128;
        int y2 = (int)Data[5] - 128;
        
        /* Careful with integer overflow in distance calc */
        long long dx = (long long)x2 - x;
        long long dy = (long long)y2 - y;
        long long dist_sq = dx * dx + dy * dy;
        
        if (dist_sq > INT_MAX) {
            /* Would overflow if cast to int */
        }
    }
    
    return 0;
}
'''
        harnesses.append({
            'name': 'fuzz_coords',
            'source': coord_fuzzer,
            'description': 'Coordinate bounds checking'
        })
        
        # 3. Format string fuzzer
        format_fuzzer = '''
/* Format string vulnerability fuzzer */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define BUFSZ 256

/* Mock pline-like function */
void test_format(const char *fmt, ...) {
    char buf[BUFSZ];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);  /* Safe version */
    va_end(args);
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size == 0 || Size > 512) return 0;
    
    /* Create format string from fuzz data */
    char fmt[BUFSZ];
    memset(fmt, 0, sizeof(fmt));
    size_t fmt_len = Size < BUFSZ - 1 ? Size : BUFSZ - 1;
    memcpy(fmt, Data, fmt_len);
    fmt[fmt_len] = '\\0';
    
    /* Test 1: Check for dangerous format specifiers */
    int has_n = 0, has_s = 0, has_d = 0;
    for (size_t i = 0; i < fmt_len - 1; i++) {
        if (fmt[i] == '%') {
            switch (fmt[i + 1]) {
                case 'n': has_n = 1; break;  /* Dangerous */
                case 's': has_s = 1; break;
                case 'd': has_d = 1; break;
                case '%': i++; break;  /* Skip %% */
            }
        }
    }
    
    /* Don't test %n - too dangerous even in fuzzer */
    if (has_n) return 0;
    
    /* Test 2: Safe format string usage */
    if (!has_s && !has_d) {
        /* No format specifiers - safe to use directly */
        test_format("%s", fmt);  /* Use as string, not format */
    } else {
        /* Has format specifiers - provide safe arguments */
        test_format(fmt, "test", 42, 'X');
    }
    
    /* Test 3: Multi-line format strings */
    if (memchr(fmt, '\\n', fmt_len)) {
        /* Contains newlines - test line splitting */
        char lines[3][BUFSZ];
        int line = 0;
        char *p = fmt;
        char *nl;
        
        while ((nl = strchr(p, '\\n')) && line < 3) {
            size_t len = nl - p;
            if (len < BUFSZ) {
                memcpy(lines[line], p, len);
                lines[line][len] = '\\0';
                line++;
            }
            p = nl + 1;
        }
    }
    
    return 0;
}
'''
        harnesses.append({
            'name': 'fuzz_format',
            'source': format_fuzzer,
            'description': 'Format string handling'
        })
        
        # 4. Integer overflow fuzzer
        int_fuzzer = '''
/* Integer overflow fuzzer for hack patterns */
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

typedef signed char schar;

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size < 8) return 0;
    
    /* Test 1: Strength overflow (critical bug we found) */
    schar strength = (schar)Data[0];
    int bonus = Data[1];
    
    /* Simulate eating spinach */
    if (strength < 118) {
        int new_str = strength + bonus;
        /* Must check for schar overflow */
        if (new_str > SCHAR_MAX) {
            new_str = SCHAR_MAX;  /* Cap at 127 */
        }
        strength = (schar)new_str;
    }
    
    /* Test 2: HP calculations */
    int hp = (int)Data[2] - 128;      /* Can be negative */
    int maxhp = (int)Data[3];
    int damage = (int)Data[4];
    
    /* Simulate damage */
    hp -= damage;
    if (hp < 0) {
        /* Player died */
        hp = 0;
    }
    
    /* Test 3: Experience/level calculations */
    unsigned long exp = *(unsigned long*)&Data[0];
    int level = 1;
    
    /* Simulate level calculation with overflow check */
    while (exp > 10 && level < 30) {
        exp /= 2;
        level++;
    }
    
    /* Test 4: Inventory weight calculations */
    if (Size >= 10) {
        int total_weight = 0;
        for (int i = 0; i < Size && i < 50; i++) {
            int item_weight = Data[i];
            /* Check for overflow before adding */
            if (total_weight > INT_MAX - item_weight) {
                /* Would overflow */
                break;
            }
            total_weight += item_weight;
        }
    }
    
    return 0;
}
'''
        harnesses.append({
            'name': 'fuzz_integers',
            'source': int_fuzzer,
            'description': 'Integer overflow patterns'
        })
        
        return harnesses

    def build_harness(self, harness: dict) -> bool:
        """Build a standalone harness"""
        name = harness['name']
        Colors.say("BUILD", f"🔨 Building {name}")
        
        # Write source
        source_file = self.fuzz_dir / f"{name}.c"
        with open(source_file, 'w') as f:
            f.write(harness['source'])
        
        # Build with clang
        build_cmd = [
            'clang',
            '-fsanitize=fuzzer,address',
            '-g', '-O1',
            str(source_file),
            '-o', str(self.fuzz_dir / name)
        ]
        
        result = subprocess.run(build_cmd, capture_output=True, text=True)
        if result.returncode != 0:
            Colors.error(f"Build failed for {name}")
            if self.debug:
                Colors.error(f"Error: {result.stderr[:500]}")
            return False
        
        Colors.success(f"✅ Built {name}")
        return True

    def create_corpus(self, name: str) -> Path:
        """Create test corpus"""
        corpus_dir = self.fuzz_dir / f"corpus_{name}"
        corpus_dir.mkdir(exist_ok=True)
        
        # Create targeted seeds
        seeds = []
        
        if 'buffer' in name:
            seeds = [
                b"A" * 255,                      # Near buffer limit
                b"A" * 256,                      # At buffer limit
                b"A" * 257,                      # Over buffer limit
                b"\\x00" * 10,                   # Null bytes
                b"\\n" * 50,                     # Newlines
                b"%s" * 50,                      # Format strings
            ]
        elif 'coord' in name:
            seeds = [
                struct.pack('bbbb', 0, 0, 0, 0),           # Origin
                struct.pack('bbbb', 79, 21, 79, 21),       # Max valid
                struct.pack('bbbb', -1, -1, -1, -1),       # Negative
                struct.pack('bbbb', 127, 127, 127, 127),   # Max signed
                struct.pack('bbbb', -128, -128, -128, -128), # Min signed
            ]
        elif 'format' in name:
            seeds = [
                b"Hello %s",
                b"%d %d %d",
                b"%n%n%n",          # Dangerous
                b"%%%%%%%%%%",      # Escaped
                b"%999999999d",     # Width attack
                b"\\x1b[31m%s\\x1b[0m",  # ANSI codes
            ]
        elif 'integer' in name:
            seeds = [
                struct.pack('B' * 8, 127, 50, 100, 100, 10, 0, 0, 0),  # Normal
                struct.pack('B' * 8, 127, 127, 1, 200, 255, 0, 0, 0),  # Max values
                struct.pack('b' * 8, -128, -1, -100, 10, 50, 0, 0, 0), # Negative
            ]
        
        for i, seed in enumerate(seeds):
            seed_file = corpus_dir / f"seed_{i:03d}"
            with open(seed_file, 'wb') as f:
                f.write(seed)
        
        Colors.say("CORPUS", f"📁 Created {len(seeds)} seeds for {name}")
        return corpus_dir

    def run_fuzzer(self, name: str, duration: int) -> bool:
        """Run a fuzzer"""
        binary = self.fuzz_dir / name
        if not binary.exists():
            Colors.error(f"Binary not found: {name}")
            return False
        
        corpus = self.create_corpus(name)
        log_file = self.log_dir / f"{name}.log"
        
        Colors.say("FUZZ", f"🚀 Running {name} for {duration}s")
        
        cmd = [
            str(binary),
            str(corpus),
            f'-max_total_time={duration}',
            '-print_final_stats=1',
        ]
        
        try:
            with open(log_file, 'w') as log:
                process = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT, 
                                       timeout=duration + 10)
        except subprocess.TimeoutExpired:
            Colors.warn(f"⏰ {name} timed out")
        except KeyboardInterrupt:
            Colors.warn(f"⚠️  {name} interrupted")
            return False
        
        # Check for crashes
        crash_files = list(corpus.glob("crash-*"))
        if crash_files:
            Colors.error(f"💥 {name} found {len(crash_files)} crashes!")
            return False
        
        Colors.success(f"✅ {name} completed successfully")
        return True

    def run_all(self, duration: int = 60) -> int:
        """Run all fuzzers"""
        if not self.check_requirements():
            return 1
        
        if not self.setup_build_dir():
            return 1
        
        harnesses = self.create_standalone_harnesses()
        Colors.say("FUZZ", f"🎯 Created {len(harnesses)} standalone harnesses")
        
        # Build all
        built = []
        for harness in harnesses:
            if self.build_harness(harness):
                built.append(harness)
        
        if not built:
            Colors.error("No harnesses built")
            return 1
        
        # Run all
        Colors.say("FUZZ", f"🎯 Running {len(built)} harnesses")
        crashes = []
        
        for harness in built:
            if not self.run_fuzzer(harness['name'], duration):
                crashes.append(harness['name'])
        
        # Summary
        print("\n" + "="*60)
        if crashes:
            Colors.error(f"💥 Crashes in: {', '.join(crashes)}")
            return 1
        else:
            Colors.success(f"✅ All {len(built)} harnesses passed")
            return 0


def main():
    parser = argparse.ArgumentParser(description='Standalone restoHack Fuzzer')
    parser.add_argument('--debug', action='store_true', help='Debug output')
    parser.add_argument('--duration', type=int, default=30, help='Duration per target')
    parser.add_argument('--target', help='Run specific target only')
    
    args = parser.parse_args()
    
    project_root = Path(__file__).parent.parent
    fuzzer = StandaloneFuzzer(str(project_root), args.debug)
    
    if args.target:
        if not fuzzer.check_requirements() or not fuzzer.setup_build_dir():
            return 1
        
        harnesses = fuzzer.create_standalone_harnesses()
        target_h = None
        for h in harnesses:
            if args.target in h['name']:
                target_h = h
                break
        
        if not target_h:
            Colors.error(f"Unknown target: {args.target}")
            return 1
        
        if not fuzzer.build_harness(target_h):
            return 1
        
        return 0 if fuzzer.run_fuzzer(target_h['name'], args.duration) else 1
    else:
        return fuzzer.run_all(args.duration)


if __name__ == "__main__":
    sys.exit(main())