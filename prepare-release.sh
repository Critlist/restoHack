#!/bin/bash
set -euo pipefail

# Prepare all release assets for GitHub releases
# Creates: static binary, source tarball, and checksums

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== restoHack Release Asset Generator ===${NC}"

# Set project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Get version from git tag or use date
VERSION="${1:-$(git describe --tags --abbrev=0 2>/dev/null || date +%Y%m%d)}"
RELEASE_DIR="release-${VERSION}"

echo -e "${YELLOW}Building release ${VERSION}...${NC}"

# Clean up any previous release directory
rm -rf "${RELEASE_DIR}"
mkdir -p "${RELEASE_DIR}"

# Get current user info (works even if not sudo)
CURRENT_UID=$(id -u)
CURRENT_GID=$(id -g)

# =============================================================================
# 1. BUILD STATIC BINARY
# =============================================================================
echo -e "\n${BLUE}[1/4] Building static binary...${NC}"

# Clean build artifacts
rm -rf CMakeCache.txt CMakeFiles cmake_install.cmake Makefile alp-build

# Run docker with user mapping to avoid permission issues
docker run --rm \
  -v "$PWD":/src \
  -w /src \
  --user "${CURRENT_UID}:${CURRENT_GID}" \
  alpine:3.20 /bin/sh -eu -c '
    # Running as user, need to handle package installation differently
    # Create a temporary script that will be run as root first
    cat > /tmp/setup.sh << "SETUP"
#!/bin/sh
apk add --no-cache build-base cmake pkgconf \
  ncurses-dev ncurses-static \
  libbsd-dev libbsd-static
SETUP
    
    echo "ERROR: Need root in container for package installation"
    echo "Switching to two-stage build..."
    exit 1
  ' 2>/dev/null || true

# Two-stage approach: First prepare environment as root, then build as user
echo -e "${YELLOW}Stage 1: Preparing build environment...${NC}"

# Create a build script that will run inside container
cat > build-static.sh << 'BUILDSCRIPT'
#!/bin/sh
set -eu

cd /src
rm -rf alp-build && mkdir alp-build && cd alp-build

# Build with static linking
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

# Prepare runtime dir
chmod 775 hackdir || true
install -d -m 775 hackdir/save || mkdir -p hackdir/save
: > hackdir/record && chmod 664 hackdir/record || true
: > hackdir/perm   && chmod 664 hackdir/perm || true

# Create launcher script
cat > run-hack.sh << "EOF"
#!/bin/sh
# restoHack launcher - ensures proper permissions
chmod 664 hackdir/record hackdir/perm 2>/dev/null || true
chmod 775 hackdir/save 2>/dev/null || true
exec ./hack "$@"
EOF
chmod +x run-hack.sh

# Strip binary to reduce size
strip hack

# Test binary
if [ -f ./hack ]; then
    ./hack -s >/dev/null 2>&1 || true
    ldd ./hack 2>&1 || echo "✓ Static binary confirmed"
    echo "Binary size: $(stat -c%s ./hack) bytes"
else
    echo "ERROR: hack binary not found!" >&2
    exit 1
fi
BUILDSCRIPT

chmod +x build-static.sh

# Run the actual build - first install packages as root, then build as user
docker run --rm \
  -v "$PWD":/src \
  -w /src \
  alpine:3.20 /bin/sh -c '
    # Install packages as root
    apk add --no-cache build-base cmake pkgconf \
      ncurses-dev ncurses-static \
      libbsd-dev libbsd-static
    
    # Add a build user matching host UID/GID
    adduser -D -u '"${CURRENT_UID}"' -g '"${CURRENT_GID}"' builduser || true
    
    # Run build as that user
    su builduser -c "sh /src/build-static.sh"
  '

# Clean up build script
rm -f build-static.sh

echo -e "${GREEN}✓ Static binary built successfully${NC}"

# =============================================================================
# 2. CREATE BINARY TARBALL
# =============================================================================
echo -e "\n${BLUE}[2/4] Creating binary distribution...${NC}"

BINARY_TAR="restoHack-${VERSION}-linux-x86_64-static.tar.gz"

# Create a clean directory for binary distribution
BINARY_DIR="${RELEASE_DIR}/binary-dist"
mkdir -p "${BINARY_DIR}"

# Copy binary and runtime files
cp alp-build/hack "${BINARY_DIR}/"
cp -r alp-build/hackdir "${BINARY_DIR}/"
cp alp-build/run-hack.sh "${BINARY_DIR}/"
cp README.md LICENSE "${BINARY_DIR}/"

# Create info file
cat > "${BINARY_DIR}/README-STATIC.txt" << EOF
restoHack ${VERSION} - Static Linux Binary

This is a statically-linked binary that should run on any x86_64 Linux system.
No dependencies required!

To run:
  ./run-hack.sh

Or directly:
  ./hack

The run-hack.sh script ensures proper permissions for save files.

Files included:
  hack         - The game executable (static binary)
  hackdir/     - Game data directory
  run-hack.sh  - Launcher script
  README.md    - Full documentation
  LICENSE      - BSD 3-Clause License

Enjoy your dungeon crawling!
EOF

# Create binary tarball
(cd "${BINARY_DIR}" && tar czf "../../${RELEASE_DIR}/${BINARY_TAR}" .)
echo -e "${GREEN}✓ Created ${BINARY_TAR}${NC}"

# =============================================================================
# 3. CREATE SOURCE TARBALL
# =============================================================================
echo -e "\n${BLUE}[3/4] Creating source distribution...${NC}"

SOURCE_TAR="restoHack-${VERSION}-source.tar.gz"

# Use git archive for clean source export (respects .gitignore)
git archive --format=tar.gz --prefix="restoHack-${VERSION}/" \
  -o "${RELEASE_DIR}/${SOURCE_TAR}" HEAD

echo -e "${GREEN}✓ Created ${SOURCE_TAR}${NC}"

# =============================================================================
# 4. GENERATE CHECKSUMS
# =============================================================================
echo -e "\n${BLUE}[4/4] Generating checksums...${NC}"

cd "${RELEASE_DIR}"

# Generate SHA256 checksums
sha256sum "${BINARY_TAR}" "${SOURCE_TAR}" > SHA256SUMS

# Also create individual checksum files (some prefer these)
sha256sum "${BINARY_TAR}" > "${BINARY_TAR}.sha256"
sha256sum "${SOURCE_TAR}" > "${SOURCE_TAR}.sha256"

# Make checksums readable
cat SHA256SUMS

echo -e "${GREEN}✓ Created SHA256SUMS${NC}"

# =============================================================================
# CLEANUP & SUMMARY
# =============================================================================
cd ..

# Clean up build artifacts (but keep alp-build for debugging if needed)
rm -rf "${RELEASE_DIR}/binary-dist"

# Extract changelog for this version
CHANGELOG_FILE="${PROJECT_ROOT}/docs/CHANGELOG.md"
CHANGELOG_CONTENT=""

if [ -f "${CHANGELOG_FILE}" ]; then
    echo -e "${BLUE}Extracting changelog for version ${VERSION}...${NC}"
    # Strip 'v' prefix if present for changelog lookup
    CHANGELOG_VERSION="${VERSION#v}"
    
    # Extract the section for this version using awk
    # Look for ## [VERSION] and get everything until the next ## [
    CHANGELOG_CONTENT=$(awk '
        /^## \['"${CHANGELOG_VERSION}"'\]/ { found=1; next }
        found && /^## \[/ { exit }
        found { print }
    ' "${CHANGELOG_FILE}")
    
    if [ -n "${CHANGELOG_CONTENT}" ]; then
        echo -e "${GREEN}✓ Found changelog entries for ${CHANGELOG_VERSION}${NC}"
    else
        echo -e "${YELLOW}⚠ No changelog entries found for ${CHANGELOG_VERSION}${NC}"
    fi
fi

# Create release notes template
cat > "${RELEASE_DIR}/RELEASE_NOTES.md" << EOF
# restoHack ${VERSION}

## Downloads

- **Linux Static Binary**: restoHack-${VERSION}-linux-x86_64-static.tar.gz
  - Statically linked, runs on any x86_64 Linux
  - No dependencies required
  
- **Source Code**: restoHack-${VERSION}-source.tar.gz
  - Full source for building on any platform
  - Requires CMake, C compiler, and ncurses

## Verification

Verify downloads with SHA256 checksums:
\`\`\`bash
sha256sum -c SHA256SUMS
\`\`\`

## What's New

${CHANGELOG_CONTENT:-"- [Add release notes here]"}

## Building from Source

\`\`\`bash
tar xzf restoHack-${VERSION}-source.tar.gz
cd restoHack-${VERSION}
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/hack
\`\`\`
EOF

# Summary
echo -e "\n${GREEN}=== Release Assets Created Successfully ===${NC}"
echo -e "Release directory: ${BLUE}${RELEASE_DIR}/${NC}"
echo ""
echo "📦 Assets ready for GitHub release:"
echo "  • ${BINARY_TAR} ($(stat -c%s "${RELEASE_DIR}/${BINARY_TAR}" | numfmt --to=iec-i --suffix=B))"
echo "  • ${SOURCE_TAR} ($(stat -c%s "${RELEASE_DIR}/${SOURCE_TAR}" | numfmt --to=iec-i --suffix=B))"
echo "  • SHA256SUMS"
echo "  • RELEASE_NOTES.md (template)"
echo ""
echo "📤 To create GitHub release:"
echo -e "  ${YELLOW}gh release create ${VERSION} ${RELEASE_DIR}/* \\\\${NC}"
echo -e "  ${YELLOW}  --title \"restoHack ${VERSION}\" \\\\${NC}"
echo -e "  ${YELLOW}  --notes-file ${RELEASE_DIR}/RELEASE_NOTES.md${NC}"
echo ""
echo "✅ All files owned by current user (${CURRENT_UID}:${CURRENT_GID})"