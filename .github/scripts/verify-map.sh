#!/bin/sh
#
# Verify that the expect smoke test log contains evidence of map rendering.
# Checks for: experience prompt, character assignment, player character,
#              walls, floor/corridors, quit prompt.
#
# POSIX sh compatible -- no bash required (works on FreeBSD, Alpine, etc.)
#
# Usage: verify-map.sh [logfile]
#   logfile defaults to /tmp/hack-expect.log
#
set -eu

LOGFILE="${1:-/tmp/hack-expect.log}"

if [ ! -f "$LOGFILE" ]; then
  echo "No expect log file at ${LOGFILE} -- cannot verify map output"
  exit 1
fi

PASS=true

echo "Checking expect log for map rendering evidence..."

# Check for experience prompt (first screen in restoHack)
if grep -q 'experienced player' "$LOGFILE"; then
  echo "  + Found experience prompt"
else
  echo "  - Missing 'experienced player' prompt"
  PASS=false
fi

# Check for character assignment
if grep -q 'Hit space to continue' "$LOGFILE"; then
  echo "  + Found character assignment screen"
else
  echo "  - Missing character assignment screen"
  PASS=false
fi

# Check for player character
if grep -q '@' "$LOGFILE"; then
  echo "  + Found player character '@'"
else
  echo "  - Missing player character '@'"
  PASS=false
fi

# Check for wall characters (horizontal or vertical)
if grep -qE '[-|]' "$LOGFILE"; then
  echo "  + Found wall characters"
else
  echo "  - Missing wall characters (- or |)"
  PASS=false
fi

# Check for floor/corridor characters
if grep -qE '[.#]' "$LOGFILE"; then
  echo "  + Found floor/corridor characters"
else
  echo "  - Missing floor/corridor characters (. or #)"
  PASS=false
fi

# Check for the quit prompt (proves game loop was running)
if grep -q 'Really quit?' "$LOGFILE"; then
  echo "  + Found 'Really quit?' prompt (game loop was responsive)"
else
  echo "  - Missing 'Really quit?' prompt"
  PASS=false
fi

echo ""
if [ "$PASS" = true ]; then
  echo "Map rendered successfully -- found experience prompt, character assignment, player (@), walls, floor/corridors, and interactive quit."
else
  echo "Map rendering verification failed. See log above for details."
  echo ""
  echo "===== Full expect log ====="
  cat "$LOGFILE"
  echo "===== End log ====="
  exit 1
fi
