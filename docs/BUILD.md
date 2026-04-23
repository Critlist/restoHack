# restoHack Build Instructions

## Quick Start

**Just want to play?** Download a pre-built binary from [Releases](https://github.com/Critlist/restoHack/releases).

**Building from source:**

```bash
git clone https://github.com/Critlist/restoHack.git
cd restoHack
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/hack
```

## Requirements

- CMake (≥3.16)
- C compiler (gcc or clang)
- ncurses library

**Install dependencies:**

- **Ubuntu/Debian:** `sudo apt install cmake build-essential libncurses-dev`
- **Fedora/RHEL:** `sudo dnf install cmake gcc ncurses-devel`
- **Arch:** `sudo pacman -S cmake gcc ncurses`
- **macOS:** `brew install cmake ncurses`
- **FreeBSD:** `pkg install cmake gcc ncurses`

## Build Types

```bash
# Release (optimized for playing)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Debug (with sanitizers for development)
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# RelWithDebInfo (production servers with debug symbols)
cmake -B build-server -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-server
```

## Platform Notes

### macOS

Xcode Command Line Tools required. Install with: `xcode-select --install`

### OpenBSD

```bash
pkg_add cmake gcc ncurses
cmake -B build -DCMAKE_C_COMPILER=/usr/local/bin/gcc
cmake --build build
```

### Static Binary

Download from releases or build on Alpine/musl with `-DCMAKE_EXE_LINKER_FLAGS="-static"`

## Troubleshooting

**ncurses not found:** Install development package (see Requirements)

**Old CMake:** Update or use traditional syntax:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

**Clean rebuild:** `rm -rf build && cmake -B build && cmake --build build`

## System Installation

`cmake --install` installs the binary, man page, and game data to the configured prefix.
The hackdir (`<prefix>/share/hack`) holds both read-only assets and writable runtime state:
high scores (`record`), save files (`save/`), and per-session lock files.

**For package maintainers** (Arch PKGBUILD, Debian rules, etc.): you must tell the build
where the hackdir will be at runtime, because the path is baked into the binary at compile
time. Pass `HACKDIR_OVERRIDE` matching your install prefix:

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DHACKDIR_OVERRIDE=/usr/share/hack
cmake --build build
cmake --install build
```

Without `HACKDIR_OVERRIDE`, the binary defaults to `<prefix>/share/hack`, which is correct
for a personal install with the default prefix (`/usr/local`). Players can always override
at runtime with `-d <path>` or the `HACKDIR` environment variable.

**Multi-user permission models** (choose one; the installer does not enforce any):

```bash
# A: World-writable — simple, appropriate for trusted machines
chmod 777 /usr/local/share/hack
chmod 1777 /usr/local/share/hack/save   # sticky: users can't delete each other's saves

# B: setgid games — traditional Unix, more controlled
chown root:games /usr/local/bin/hack /usr/local/share/hack
chmod g+s /usr/local/bin/hack           # binary runs as games group
chmod 775 /usr/local/share/hack
chmod 664 /usr/local/share/hack/record /usr/local/share/hack/perm
chmod 2770 /usr/local/share/hack/save   # setgid: new files inherit games group

# C: Per-user override — zero shared state, no permissions needed
# Each player runs: hack -d ~/.hack   (no shared high scores)
```

Note: hackdir root must be writable by the game process (bones files and session lock
files land there alongside the read-only assets).

## Development

The project uses standard CMake with presets for modern IDEs (VSCode, CLion, etc).

The `debug` and `relwithdebinfo` presets automatically set `HACKDIR_OVERRIDE` to the
build tree, so development builds find their data without a `-d` flag. The `release`
preset does not set this, since release binaries are intended for system installation.

For contributing, see [CODING_STANDARDS.md](CODING_STANDARDS.md).
