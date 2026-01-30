#!/bin/sh
set -euo pipefail

# Capture real user info before potential sudo elevation
REAL_USER="${SUDO_USER:-$(whoami)}"
REAL_UID="${SUDO_UID:-$(id -u)}"
REAL_GID="${SUDO_GID:-$(id -g)}"

rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile alp-build

docker run --rm --network=host \
  -v "$PWD":/src \
  -w /src \
  alpine:3.20 /bin/sh -eu -c '
    apk add --no-cache build-base cmake pkgconf \
      ncurses-dev ncurses-static \
      libbsd-dev libbsd-static

    rm -rf alp-build && mkdir alp-build && cd alp-build
    # Set cleanup trap
    trap "chmod -R 755 /src/alp-build 2>/dev/null || true" EXIT

    # Check what static libraries are actually available
    echo "Available static curses libraries:"
    ls -la /usr/lib/lib*ncurses* /usr/lib/lib*curses* 2>/dev/null || true
    
    # Force static linking with explicit curses library paths
    cmake .. \
      -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -DCMAKE_FIND_LIBRARY_SUFFIXES=".a" \
      -DCMAKE_EXE_LINKER_FLAGS="-static" \
      -DCMAKE_C_FLAGS="-D_STATIC_BUILD" \
      -DHACKDIR_OVERRIDE=hackdir \
      -DCURSES_FOUND=TRUE \
      -DCURSES_INCLUDE_DIR=/usr/include \
      -DCURSES_INCLUDE_DIRS=/usr/include \
      -DCURSES_LIBRARY=/usr/lib/libncursesw.a \
      -DCURSES_LIBRARIES=/usr/lib/libncursesw.a \
      -DCURSES_HAVE_CURSES_H=1 \
      -DCURSES_HAVE_NCURSES_H=1 \
      -DHAVE_DELAY_OUTPUT=1

    make -j"$(nproc)"

    # Prepare runtime dir with sane perms
    chmod 775 hackdir
    install -d -m 2775 hackdir/save   # group-writable + setgid
    : > hackdir/record && chmod 664 hackdir/record
    : > hackdir/perm   && chmod 664 hackdir/perm

    # launcher
    cat > run-hack.sh << 'EOF'
#!/bin/sh
# Ensure writable state files, ignore errors in read-only envs
chmod 664 hackdir/record hackdir/perm 2>/dev/null || true
chmod 2775 hackdir/save 2>/dev/null || true
exec ./hack "$@"
EOF
    chmod +x run-hack.sh

    # Debug: show what files exist
    echo "Files in build directory:"
    ls -la ./
    
    # quick smoke test
    if [ -f ./hack ]; then
        ./hack -s >/dev/null 2>&1 || true
        
        # prove static - show what is linked for debugging
        echo "Checking binary linkage:"
        ldd ./hack 2>&1 || echo "Static binary confirmed - ldd cannot analyze static binaries"
        
        echo "Binary size: $(stat -c%s ./hack) bytes"
        
        # Copy source files needed for building
        cp /src/CMakeLists.txt /src/config.h.in .
        cp -r /src/src .
        
        # Copy packaging files for CPack compatibility
        cp /src/README.md /src/LICENSE .
        
        # Copy asset directories needed for CMake setup
        cp -r /src/data /src/help /src/hh /src/rumors .
        
        # reproducible-ish hybrid tarball (binary + source)
        TAR=restoHack-static-$(date +%Y%m%d)-linux-x86_64.tar.gz
        tar --numeric-owner -czf "../$TAR" hack hackdir/ run-hack.sh CMakeLists.txt config.h.in src/ README.md LICENSE data help hh rumors
        echo "Created hybrid $TAR successfully in project root!"
    else
        echo "ERROR: hack binary not found!" >&2
        exit 1
    fi
  '

# Fix ownership of created tarball if running under sudo
if [ -n "${SUDO_USER:-}" ]; then
    TAR_FILE="restoHack-static-$(date +%Y%m%d)-linux-x86_64.tar.gz"
    if [ -f "$TAR_FILE" ]; then
        echo "Fixing ownership of $TAR_FILE to ${REAL_USER}:${REAL_GID}"
        chown "${REAL_UID}:${REAL_GID}" "$TAR_FILE"
    fi
fi
