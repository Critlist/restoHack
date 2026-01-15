# Changelog

All notable changes to restoHack will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased] - Windows Port

### Added

- **Windows Support**: Native Windows builds via PDCursesMod integration
  - Vendored PDCursesMod 4.5.4 as git submodule at `windows/pdcurses/`
  - PDCurses termcap shim for Windows Console API compatibility
  - File locking via Windows LockFileEx (replaces Unix flock)
  - Console control handler for Ctrl+C and window close events
  - UTF-8 console setup for box-drawing character rendering
  - POSIX compatibility shims (getuid, getpid, usleep)

### Changed

- **Build System**: Isolated Windows code into dedicated `windows/` directory
  - `windows/win32.c` - PDCurses termcap shim + file locking implementation
  - `windows/win32.h` - Windows API declarations and POSIX shims
  - `windows/pdcurses/` - Vendored PDCursesMod 4.5.4 (git submodule)
  - All Windows code guarded with `#ifdef _WIN32` or `if(WIN32)`
- **Static Linking**: Windows builds produce portable .exe with minimal dependencies
  - MSVC: Static runtime (/MT) eliminates vcruntime140.dll dependency
  - MinGW: Selective static linking (-static-libgcc -static-libstdc++)

### Technical

- CMake conditional compilation ensures Unix builds unaffected
- Generator expressions for platform-specific include paths and linking
- Submodule initialization check with clear error message and instructions

## [1.1.5] 2025-12-12

### Changed

- **SAVE FORMAT**: Save version incremented from 1 to 2 for safe pointer serialization
  - Pointers in `struct you` now serialized as semantic IDs instead of raw memory addresses
  - `usick_cause` (sickness cause string) saved as object type index or special marker
  - `uprops[].p_tofn` (timeout function pointers) saved as function IDs
  - Struct dump now zeroes all pointer fields before writing
  - Version 1 saves automatically migrate to Version 2 on load
  - Pre-versioned saves (no magic number) now rejected with clear error message
  - Fixes segfault risk in multi-user server environments (ASLR, process address space changes)

### Fixed

- **SECURITY**: Pointer serialization vulnerabilities in save/restore system
  - `usick_cause` pointer became invalid after restore, causing segfaults on timeout
  - Function pointers in `uprops[]` saved as raw addresses, invalid on restore
  - Raw pointers in struct dump could cause undefined behavior across process invocations
  - Added post-restore validation for state consistency (Sick flag vs usick_cause)
  - Graceful handling of corrupted save data (fallback to safe defaults, no crashes)

## [1.1.4] - 2025-09-11

### Fixed

- **CRITICAL**: Fixed save/restore crash when accessing inventory after restore
  - Object tail strings (scroll labels, etc.) weren't NUL-terminated after restore
  - Class table pointers (oc_name, oc_descr) became stale after restore, pointing to old process addresses
  - Now properly allocates extra byte for NUL termination on restore
  - Preserves static string pointers from binary while maintaining shuffled descriptions
  - Added range checks for object name length and NULL guards for string operations

### Changed

- Updated README features section

## [1.1.3] - 2025-01-10

### Fixed

- **CRITICAL**: Fixed "Cannot get status of hack" error on Ubuntu systems
  - Bug prevented game from running when invoked with path (./hack, /usr/bin/hack)
  - Caused by security validation modifying argv[0] used for PATH resolution
  - Affected Ubuntu 22.04/24.04 but not Arch Linux due to invocation differences
  - Fix preserves original argv[0] for PATH search while validating basename separately

## [1.1.2] - 2025-01-05

### Changed

- **DOCUMENTATION**: Updated HISTORY_OF_HACK.md to include Hack 1.0.2 (April 1985) in timeline
- **DOCUMENTATION**: Updated references to cite original Linux.com article by Julie Bresnick (2000)
- **DOCUMENTATION**: Reduced restoHack content in history document to maintain focus on Hack's history
- **DOCUMENTATION**: Removed emojis from all documentation for consistency
- **DOCUMENTATION**: Removed incorrect save compatibility claims due to flock implementation

### Fixed

- **BUILD**: Fixed string overflow warning in hack.topl.c pline() function by adding n0 > 0 check

## [1.1.1] - 2025-08-28

### Added

- **Comprehensive security audit**: Systematic review and hardening of all game logic files
- **Portability layer**: Added compat.h for BSD function compatibility

### Fixed

- **SECURITY**: Fixed 150+ array bounds violations across all game systems
- **SECURITY**: Added bounds checking to prevent buffer overflows
- **SECURITY**: Fixed null pointer dereferences in monster, object, and room systems  
- **SECURITY**: Protected against integer overflow in tracking and scoring systems
- **SECURITY**: Secured format string vulnerabilities in error reporting
- **SECURITY**: Fixed memory corruption issues in worm segments and vault guards
- **CRITICAL**: Strength overflow bug that could instantly kill players (eating spinach, potions)
- **CRITICAL**: Coordinate sign conversion bugs causing wraparound issues
- **BUILD**: Fixed format-nonliteral warnings with pragma directives
- **BUILD**: Resolved index/rindex macro collisions with variable names

### Changed

- Removed index/rindex macros from hack.h, moved to compat.h
- Renamed 'index' variable to 'idx' in makedefs.c to avoid collisions
- Added format string validation to block dangerous %n specifier

## [1.1.0] - 2025-08-23

### Added

- **Terminal resize protection**: SIGWINCH handler prevents display corruption from dynamic terminal sizing

### Fixed

- **CRITICAL**: Terminal resize corruption causing map display bugs and wall-walking glitches
- **CRITICAL**: Message buffer concatenation overflow with wide terminals (CO > 80)
- **CRITICAL**: --More-- prompt positioning bugs in topline system
- **SECURITY**: Eliminated all 151 char-subscript buffer underflow vulnerabilities
- **SECURITY**: Fixed 30 strict-aliasing undefined behavior violations
- **SECURITY**: Resolved all uninitialized variable warnings with -Werror gates
- **SECURITY**: Fixed 10 control-reaches-end-of-non-void-function undefined behavior issues
- **SECURITY**: Fixed 16 implicit-fallthrough undefined behavior cases
- **SECURITY**: Disabled dangerous fuzzing functions that could create undeletable files

### Changed

- Updated all signed char array indexing to safe unsigned char casting
- Enhanced EGD macro in hack.vault.c with proper function wrapper
- Added proper return statements to all void functions
- Implemented fallthrough annotations for intentional switch cases
- Configured MSan with non-aborting mode for uninstrumented library compatibility

## [1.0.6] - 2025-08-14

### Added

- Historical references and fixed build config in CMake

## [1.0.5] - 2025-08-12

### Added

- Coding standards documentation for non-original code contributions
- Historical source files and game commands documentation

### Changed  

- Refactored memory allocation to ANSI C standards
- Improved asset copying logic to handle files uniformly with better error messaging
- Updated CMake and CI configuration for BSD builds
- Enhanced CPack configuration with optional packaging support

### Technical

- Preserved original 1984 behavior while eliminating casting requirements in memory allocation
- Enhanced build system reliability across different platforms

## [1.0.4] - 2025-08-11

### Added

- Hybrid binary+source tarballs containing both static executable and source code
- BSD-specific build instructions for FreeBSD, OpenBSD, and NetBSD
- Support for building from hybrid tarballs without separate source download
- CI testing for published hybrid tarballs on release tags
- FreeBSD CI reliability improvements

### Fixed

- Shell syntax error in static-build.sh (apostrophe in comment breaking Docker string)
- BusyBox tar compatibility issues with GNU tar-specific options
- Missing source files in static binary releases preventing user rebuilds
- CI flakes in FreeBSD environment by removing interactive game execution
- TLS certificate issues in CI with ca_root_nss package addition

### Changed

- Static build script now includes CMakeLists.txt, config.h.in, and src/ directory
- Updated documentation with hybrid tarball usage instructions
- Improved cross-platform build documentation
- Enhanced CI with hard failure tests for required files instead of soft warnings

## [1.0.3] - 2025-08-10

### Added

- Portable RNG seeding with CMake feature detection
- Support for arc4random, arc4random_buf, getentropy, and srandomdev across platforms
- Proper curses library detection replacing hardcoded termcap linking
- Fallback delay_output implementation for systems without curses version

### Fixed

- Build failures on NetBSD/pkgsrc due to missing delay_output symbol
- Function name conflicts with curses library (unctrl, timeout, delay_output)
- OS-specific #ifdef dependencies that broke portability
- Hardcoded termcap library detection causing link failures

### Changed

- Replaced OS-specific `#ifdef __linux__` with portable feature detection
- Updated CMake to use `find_package(Curses REQUIRED)`
- Renamed internal functions to avoid curses conflicts: `unctrl()` → `hack_unctrl()`, `timeout()` → `hack_timeout()`
- Enhanced static build script with better library detection and quality checks

### Technical

- All original 1984 code preserved via comments, not deleted
- Added CMake feature detection for RNG functions
- Improved static binary build process with libbsd support
- Universal build compatibility across Linux, NetBSD, macOS

## [1.0.2] - 2025-08-08

### Added

- Static binary release for Linux x86_64
- Prebuilt distribution with no external dependencies
- Enhanced static build pipeline with Docker-based compilation

### Changed

- Updated static build script for better reliability and reproducibility
- Improved packaging with proper permissions and launcher script

### Technical

- Static linking against ncurses for zero-dependency distribution
- Optimized binary size and performance

## [1.0.1] - 2025-08-06

### Added

- Modern flock()-based locking system replacing unreliable 1984 link() locking
- Automatic cleanup of stale lock files on process death
- Enhanced save system with versioned directories and backup support

### Fixed

- "Game in progress" hangs caused by stale lock files
- Filesystem compatibility issues with modern systems
- Segfault in death screen (hack.rip.c) caused by buffer overflow in name centering
- RIP screen display issues with various username lengths

### Changed

- Complete K&R to ANSI C conversion for entire codebase (~250 functions)
- Updated read-only string literals to writable arrays for compiler compatibility
- Improved error handling and memory safety throughout

### Technical

- Preserved 100% authentic 1984 gameplay mechanics and balance
- Enhanced build system with CMake replacing vintage Makefiles
- Modern signal handling for Unix window environments

## [1.0.0] - 2025-08-01

### Added

- Initial public release of restoHack
- Complete playable restoration of 1984 Hack game
- All core game systems: character creation, inventory, combat, magic, shops
- Full dungeon generation with authentic maze algorithms
- Save/load system with proper exit handling
- Terminal display with complete termcap support

### Technical

- CMake-based build system for modern compatibility
- Cross-platform support (Linux, macOS, BSD)
- POSIX-only dependencies for maximum portability
- Complete preservation of original 1984 game logic
- Zero external dependencies beyond standard C library and curses

### Game Features

- Multiple character classes: Tourist, Wizard, Fighter, Caveman
- Authentic item generation and cursed/blessed mechanics
- Monster AI including pets, shopkeepers, and guards
- Magic system with potions, scrolls, and wands
- Shop system with authentic billing mechanics
- Permadeath with proper tombstone generation
