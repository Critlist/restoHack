# restoHack: Bringing 1984 Hack Back from the Dead

**restoHack** is a full-blooded resurrection of *Hack*, the 1984 roguelike that spawned *NetHack*.
Not a remake. Not a reboot. This is *software preservation with a blowtorch and a scalpel*.

The goal: make the original code compile, run, and dungeon-crawl exactly as it did when floppy was King.

Think resto-mod: the soul stays vintage, the internals get a precision rebuild.

**Philosophy**: "Fix what breaks, preserve what works" - authentic 1984 gameplay with modern build system and safety improvements.

**[Read the Complete History of Hack](docs/HISTORY_OF_HACK.md)**  
*From Rogue (1980) to NetHack's rise, through decades of digital decay, to restoHack's 2025 resurrection*

---

<h3 align="Left">Screenshots</h3>
<p align="center"><em>Luck and death, in that order.</em></p>

<p align="center">
  <a href="https://github.com/Critlist/restoHack/releases">
    <img src="docs/media/FullMoon.png" alt="Full moon event" width="420"/>
  </a>
  <a href="https://github.com/Critlist/restoHack/releases">
    <img src="docs/media/Tombstone.png" alt="Tombstone" width="420"/>
  </a>
</p>

---

## Features

* **Modern Build System** – CMake replaces Makefiles. Clean builds on Linux, FreeBSD, and other Unix systems.
* **Authentic Systems** – Over a dozen original systems restored from source, untouched in design.
* **Complete K\&R Modernization** – Entire codebase converted to ANSI C (\~250 functions updated).
* **Unix/Linux Fixes** – No more hardcoded BSD paths or FS quirks.
* **Cross-Platform Verified** – Works on Linux (glibc/musl) & FreeBSD; CI-tested.

> **Tested on:** Arch, Alpine, FreeBSD 14.3 and WSL.
> macOS support pending. Arch users can install via the AUR.

---

 **Recognition:**  
RestoHack was recognized by **GitHub** as a *For the Love of Code* category winner (2025).  
Featured in the official GitHub Blog:  
[From Karaoke Terminals to AI Resumes — The Winners of GitHub’s For the Love of Code Challenge](https://github.blog/open-source/from-karaoke-terminals-to-ai-resumes-the-winners-of-githubs-for-the-love-of-code-challenge/)

---

**Development Status:**  
After a brief pause, active development is gearing back up. The upcoming work will focus heavily on strengthening safety guards throughout the codebase and continuing the long-term mission of cleaning up undefined behavior while keeping the original 1984 
spirit intact.



## Installation

### Arch Linux (AUR)

```bash
yay -S restohack
```

## Build from Source (Pre-release)

**Requirements:** `git`, `cmake`, a C compiler, `ncurses`

```bash
git clone https://github.com/Critlist/restoHack.git
cd restoHack
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ./hack
```

**Alternative** (if your system supports CMake presets):

```bash
cmake --preset=release && cmake --build build
```

### Windows

Requires git submodule initialization (one-time):

```bash
git submodule update --init --recursive
```

**Visual Studio 2022:**
```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

**MinGW:**
```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binary location: `build/Release/hack.exe` (MSVC) or `build/hack.exe` (MinGW)

---

### Download Pre-built Binary (Linux)

Download the static binary from [Releases](https://github.com/Critlist/restoHack/releases):

```bash
mkdir -p ~/Games/restohack
cd ~/Games/restohack
tar -xzf restoHack-*-linux-x86_64-static.tar.gz
./run-hack.sh
```

*Note: As of v1.1.1, we provide separate binary and source tarballs instead of hybrid packages.*

*(static binary, no dependencies required)*

**For BSD systems, development builds, IDE integration, and troubleshooting:** see [**Build Instructions**](docs/BUILD.md)

---

## Current Status

v1.1.5 — Stable Release
Introduces a fully overhauled save file system. Inspired by NetHack and EvilHack, the new format improves safety, portability, and long-term compatibility across platforms and builds.

---

## Recent Fixes

* **Save System Safety** – Version 2 save format with pointer serialization
* **Ubuntu Fix** – Resolved PATH resolution bug preventing game launch on Ubuntu 22.04/24.04
* **Security Audit** – Fixed 150+ vulnerabilities: buffer overflows, null pointers, format strings
* **Terminal Resize** – Added SIGWINCH handler to prevent display corruption on window resize
* **40-Year Bug** – Fixed strength overflow that could instantly kill players (spinach/potions)

---

## Preservation Philosophy

Fix what breaks, preserve what works.

---

## Source Code

Built from Andries Brouwer's Hack 1.0.3 (1985) preserved in FreeBSD's games collection. Original source in [`docs/historical/original-source/`](docs/historical/original-source/)

See [CODING_STANDARDS.md](docs/CODING_STANDARDS.md) for how changes are documented.

Full history: [HISTORY_OF_HACK.md](docs/HISTORY_OF_HACK.md)

---

## Gameplay

You’re the `@`, diving into the Mazes of Menace to steal the Amulet of Yendor.
Expect monsters, magic, cursed loot, and permadeath. Controls are Vi-style (`hjkl`). Survival is… unlikely.

---

## Contributing

Bug fixes, portability patches, packaging help—welcome.
Want to port to Plan 9? Go for it. Just document changes and respect the code.

---

## License

3-Clause BSD. Do what you want, just don’t sue me. See `LICENSE`.
