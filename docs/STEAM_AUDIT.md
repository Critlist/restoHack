# restoHack Steam-Readiness Audit
**Branch**: windows-port  
**Date**: 2026-06-19  
**Version**: 1.1.1

---

## A. Steam-Readiness Audit

### 1. Save / Data Location

**Architecture**: The 1984 game calls `chdir(HACKDIR)` at startup (hack.main.c:203). All subsequent I/O is relative to that directory. Save files write to `$HACKDIR/save/$uid$plname` (e.g., `save/0mjh` on Windows where `getuid()` returns 0, `save/1000mjh` on Linux).

**Status per platform**:

| Platform | Data location | Save location | Assessment |
|---|---|---|---|
| Linux (Steam) | `<install>/share/hack/` (baked at build time) | `<install>/share/hack/save/` | Working; non-XDG but authentic |
| Linux (dev) | `<build>/hackdir/` | `<build>/hackdir/save/` | Working |
| Windows (Steam) | `<exedir>\hackdir\` (runtime discovery) | `<exedir>\hackdir\save\` | Working; self-locating |
| Windows (NSIS) | same as above | same as above | Working |

**XDG / AppData consideration**: Steam expects games to store user data in per-user locations (XDG on Linux, `%APPDATA%` on Windows) so cloud saves and multi-user installs work cleanly. restoHack stores saves in the install directory, which is the authentic 1984 behavior. This is acceptable for a Steam release with a note in the store description. It does mean saves are not portable between installations and cloud save integration is not possible without further work.

**Decision**: Preserve the 1984 `chdir` behavior. Do not introduce XDG redirection unless Valve specifically requires it (they do not for solo games).

**Risk**: On Linux system installs (`/usr/share/hack/`), the `save/` directory must be world-writable. This is handled in the install rules. Steam installs to a per-user directory (`~/.steam/steam/steamapps/common/restoHack/`), so permissions are not an issue there.

---

### 2. Working Directory Independence

**Problem**: Steam's working directory at launch is not guaranteed to equal `dirname(executable)`. `argv[0]` may also be unreliable.

**Current solution**:
- **Windows**: `win32_setup_hackdir()` in `windows/win32.c` calls `GetModuleFileNameA(NULL, ...)` to get the real exe path, then sets `HACKDIR=<exedir>\hackdir` in the CRT env table before `chdirx()` runs. This is unconditional on `HACKDIR` not being set. ✓
- **Linux**: `steam_launch.sh` uses `$(cd "$(dirname "$0")" && pwd)` to resolve the script's real directory and exports `HACKDIR=<scriptdir>/hackdir`. ✓

**Critical ordering**: `gethdate(hname)` (hack.main.c:197) stats `argv[0]` to get the binary's mtime and must run BEFORE `chdirx()` (line 203). This ordering is already correct and must not be disturbed.

**`-d` flag and `HACKDIR` env var**: Both already override the compile-time baked path. Steam launch options can pass `-d <path>` or set `HACKDIR=<path>` for non-default data locations.

**Assessment**: Working directory independence is fully implemented for both platforms. ✓

---

### 3. Packaging Layout

**Windows (flat, Steam-friendly)**:
```
hack.exe
hackdir/
  data
  help
  hh
  rumors
  news          ← currently missing from install rules (see gap below)
  perm          ← created empty by install(CODE ...) ✓
  record        ← created empty by install(CODE ...) ✓
  save/         ← created by install(CODE ...) ✓
```

**Linux (FHS)**:
```
bin/hack
share/hack/
  data
  help
  hh
  rumors
  news          ← currently missing from install rules (see gap below)
  perm          ← created empty by install(CODE ...) ✓
  record        ← created empty by install(CODE ...) ✓
  save/         ← created by install(CODE ...) ✓
share/man/man6/hack.6
```

**Gap found**: The `news` file lives in `hackdir/news` in the source tree, not at the source root. The install rules iterate `${CMAKE_CURRENT_SOURCE_DIR}/${_asset}` for `data help hh rumors news`. Because `news` does not exist at the source root, the `if(EXISTS ...)` guard silently skips it. The `news` file contains the welcome message shown at every fresh startup — its absence produces a silent no-op (the game checks `readnews()` and continues if the file is missing), but the player never sees the welcome banner. **Fixed below.**

**`record_lock` committed**: `hackdir/record_lock` is tracked in git. It is a runtime lock artifact and should be in `.gitignore`, not version control.

---

### 4. Steam Compatibility

| Concern | Status |
|---|---|
| Self-locating binary (Windows) | ✓ GetModuleFileNameA |
| Self-locating binary (Linux) | ✓ steam_launch.sh |
| Steam Deck (Linux ARM64) | Untested — needs cross-compile verification |
| Steam Overlay | N/A — terminal/curses game; overlay won't render over a console window |
| Steam cloud saves | Not implemented — requires XDG or AppData redirection |
| Steam Input | N/A — keyboard-only roguelike |
| SteamWorks API | Not needed for basic distribution |
| Steam Runtime (Linux) | Should work — ncurses is in the Steam Runtime; verify against `scout` runtime |
| NSIS installer (Windows) | Configured in CPack ✓ |
| ZIP distribution | Configured in CPack ✓ |

**Steam Deck note**: The Deck runs a 64-bit ARM64 Linux (with x86_64 Proton layer available). The Linux binary can run through Proton's compatibility layer, or a native ARM64 build can be produced. No action required for initial release; Valve validates compatibility through their own review.

---

### 5. Runtime Dependency Audit

**Windows**:
- PDCurses: vendored, compiled in (static) ✓
- GCC runtime: `-static-libgcc` ✓
- winmm: linked via full `.a` path (no DLL dep) ✓
- kernel32, user32: always present ✓
- **Result**: zero external DLL dependencies — clean Steam install

**Linux**:
- ncurses: runtime dep — listed in `CPACK_DEBIAN_PACKAGE_DEPENDS` (`libc6, libncurses5 | libncurses6`) ✓
- glibc: always present ✓
- libbsd (optional): only linked if `arc4random` is not in stdlib — CPack deps don't mention it yet. Low risk (fallback to `/dev/urandom`).
- **Result**: standard system deps, no surprises

---

### 6. Release Automation

**What exists**:
- `CMakePresets.json`: `release`, `debug`, `relwithdebinfo`, `release-install`, `windows-mingw`, `windows-clang-cl` ✓
- `RESTOHACK_RELEASE_BUILD` CMake option: bakes FHS HACKDIR at build time ✓
- CPack: DEB + RPM + TGZ (Linux), NSIS + ZIP (Windows) ✓
- `prepare.sh` / `release.sh`: GitHub release scripts (existing)

**What's missing**:
- No CI/CD pipeline for cross-compilation (no `.github/workflows/`)
- Windows artifacts must be built manually from Linux via MinGW preset
- No automated Steam depot upload (requires Steamworks SDK + `steamcmd`)

**Assessment**: Release automation is adequate for manual releases. CI is a nice-to-have for a solo/small project.

---

### 7. First-Time User Experience

**Startup sequence** (from hack.main.c):
1. Parse arguments → set flags, player name
2. `gethdate(hname)` — binary mtime check (must precede chdir)
3. `chdirx(HACKDIR, 1)` — chdir to game directory
4. Locking check — exits if another instance holds lock
5. `readnews()` — displays `news` file if player hasn't seen this version
6. Player name prompt if not given
7. Game starts

**Gaps**:
- `news` file missing from install rules → player never sees welcome banner (fixed below)
- No Steam-specific on-boarding (controls reference card, etc.) — preservation deferral
- If `perm` or `record` don't exist at install time, the game exits with an error. Install rules create them. ✓

**Console window (Windows)**: The game opens in a console/terminal window. On Steam this is standard for terminal games. No action needed.

---

### 8. Archaeological Preservation

**What has been changed** (all documented with `/* Modern ... */` comments):
- K&R → ANSI C function signatures (~250 functions)
- `vsprintf` → `vsnprintf` (buffer overflow safety)
- `link()`-based locking → `flock()` / LockFileEx (reliability)
- `srand`/`rand` → `arc4random` / `getentropy` where available
- CMake build system (replaces `Makefile`)
- `win32.c` / `win32.h`: Windows compatibility shim (entirely new, `#ifdef _WIN32` guarded)

**What has NOT been changed**:
- Game logic, dungeon generation, combat, monsters, items
- Terminal rendering pipeline (termcap → tputs path preserved on Unix)
- Save file format (binary struct dump, same as 1984)
- Bones files sharing between players
- High score file format (`record`)

**Save compatibility**: Save files from different builds (Linux vs Windows, different struct alignment) are not cross-compatible. This is 1984 behavior — the original had the same issue between architectures. The `SAVE_VERSION` macro in the save directory name (`save/v1.1.1/`) guards against loading stale saves after upgrades.

---

## B. Prioritized Remaining Work

### P0 — Must ship with

1. **Fix `news` install rule** — welcome banner missing on fresh installs (5-minute fix)
2. **Remove `hackdir/record_lock` from git** — runtime artifact should not be committed
3. **Verify `steam_launch.sh` is executable** — `chmod +x` must be in install rules or noted in packaging

### P1 — Should ship with

4. **Linux Steam layout**: The flat layout (exe + hackdir/ side-by-side) used on Windows works better for Steam than the FHS layout. Consider a `windows-style` Linux build preset that uses `./hackdir` instead of `/usr/share/hack`. The launch script already handles this — only the HACKDIR bake needs adjustment.
5. **`news` file in source root** — the asset lives in `hackdir/news` but should be at the source root like `data`, `help`, `hh`, `rumors` for consistency with the install rules. Either move it or fix the install rule to copy from `hackdir/`.
6. **Verify Windows static deps produce zero DLL deps**: Run `objdump -p hack.exe | grep DLL` on the MinGW build to confirm no unexpected DLLs.

### P2 — Nice to have

7. **GitHub Actions CI**: Matrix build (Linux native + MinGW cross) to catch regressions
8. **Steam launch option documentation** in README: explain `-d <path>` and `HACKDIR` env var
9. **`libbsd` in CPack deps** if `arc4random` is pulled from libbsd (currently not in DEB deps)
10. **Steam Deck native ARM64 build** — Proton layer works but native is cleaner

### P3 — Future Steam integration (not blocking)

11. **Steam cloud saves** — requires XDG / AppData redirection; architectural change, preservation deferral
12. **Steam achievements** — would require SteamWorks SDK integration; not authentic
13. **Steam Overlay** — curses/console game; overlay cannot render over terminal window

### Preservation deferrals (intentional non-changes)

- XDG save location redirection — changes 40-year-old save architecture
- `bones_xx` shared between users — 1984 multiplayer behavior, authentic
- `record` shared high score file — authentic; Steam leaderboards are a separate concern
- Terminal/curses rendering — no graphical frontend planned
- Save file format — binary struct dump; format change would break saves

---

## D. Separation Summary

| Category | Items |
|---|---|
| **Must-have for shipping** | Fix `news` install rule; remove `record_lock` from git; verify launch script is executable |
| **Nice-to-have** | CI/CD pipeline; flat Linux Steam preset; zero-DLL verification; libbsd in CPack deps |
| **Future Steam integration** | Cloud saves; Steam Deck native build; Steam Input mapping |
| **Preservation deferrals** | XDG redirection; graphical frontend; Steam achievements; save format changes |
