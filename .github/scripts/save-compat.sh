#!/bin/sh
#
# Save format v3: a game saved by one build must restore in another.
# Saves with <first>, restores with <second> (both under ASan/UBSan in CI),
# and checks that the final save is a v3 file still holding the pet (a
# little dog, stable monster ID 1004; a new game always starts with one),
# that nothing was kept aside, and that neither run printed a sanitizer
# report.
#
# Usage: save-compat.sh <first-build-dir> <second-build-dir>
#
set -eu

FIRST="$1"
SECOND="$2"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
HD="$(mktemp -d)"
trap 'rm -rf "$HD"' EXIT
cp -R "$FIRST/hackdir/." "$HD/"
rm -f "$HD"/save/[0-9]*

export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1}"

# A tame little dog record: int xl = sizeof(struct edog) (40, little-endian
# on the x86_64/aarch64 CI runners), then the big-endian ID 1004 (0x3ec).
has_pet() {
  LC_ALL=C grep -qaP '\x28\x00\x00\x00\x00\x00\x03\xec' "$1"
}

FAIL=0
rc=0
perl -e 'alarm 120; exec @ARGV' \
  expect -f "$SCRIPT_DIR/save-compat.exp" "$FIRST/hack" "$SECOND/hack" "$HD" || rc=$?
cat /tmp/hack-save-compat.log | tr -d '\000' | tail -c 2000 || true
echo
if [ "$rc" -ne 0 ]; then
  echo "FAIL: expect exited with $rc"
  FAIL=1
fi

SAVE="$(ls "$HD"/save/[0-9]* 2>/dev/null | head -1 || true)"
if [ -z "$SAVE" ]; then
  echo "FAIL: no save file after the second run"
  FAIL=1
else
  if [ "$(head -c 4 "$SAVE")" != "RHCK" ]; then
    echo "FAIL: $SAVE is not a save format v3 file"
    FAIL=1
  fi
  if has_pet "$SAVE"; then
    echo "OK: pet present after restore"
  else
    echo "FAIL: pet missing from the save after restore"
    FAIL=1
  fi
fi
if ls "$HD"/save/*.outdated* >/dev/null 2>&1; then
  echo "FAIL: the save was kept aside instead of restored"
  FAIL=1
fi
if grep -aqE 'ERROR: AddressSanitizer|runtime error|ERROR: LeakSanitizer' \
  /tmp/hack-save-compat.log; then
  echo "FAIL: sanitizer report"
  FAIL=1
fi
if [ "$FAIL" -ne 0 ]; then
  exit 1
fi
echo "PASS: saved with $FIRST, restored with $SECOND"
