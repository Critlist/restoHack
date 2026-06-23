#!/bin/sh
# restoHack Steam launcher for Linux
#
# Steam sets the working directory to the game's install folder before
# launching, but it does not guarantee $PWD == dirname($0).  This script
# resolves the game directory from its own real path so the HACKDIR is
# always correct regardless of how Steam invokes it.
#
# Steam launch option: set to "run %command%" if you use this script as
# the primary executable, or configure the launch options in Steam to run
# this script as a wrapper.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HACKDIR="${SCRIPT_DIR}/hackdir"

# Create the save directory and sentinel files on first run.
# This is safe to repeat and ensures fresh Steam installs work.
mkdir -p "${HACKDIR}/save"
: > "${HACKDIR}/perm"    2>/dev/null || true
: > "${HACKDIR}/record"  2>/dev/null || true
chmod 664 "${HACKDIR}/perm" "${HACKDIR}/record" 2>/dev/null || true
chmod 775 "${HACKDIR}/save" 2>/dev/null || true

# Resolve the binary (may differ from script name in some Steam configs)
HACK_BIN="${SCRIPT_DIR}/hack"
if [ ! -x "${HACK_BIN}" ]; then
    echo "restoHack: cannot find executable at ${HACK_BIN}" >&2
    exit 1
fi

exec env HACKDIR="${HACKDIR}" "${HACK_BIN}" "$@"
