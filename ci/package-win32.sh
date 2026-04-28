#!/bin/sh
# Package the Windows x86 (32-bit) build into a .zip archive.
#
# Environment variables:
#   BUILDDIR - build output directory  (default: build/win32)
#   DISTDIR  - package output directory (default: dist)
set -e

BUILDDIR="${BUILDDIR:-build/win32}"
DISTDIR="${DISTDIR:-dist}"
ARCHIVE="$DISTDIR/fed-win32.zip"

test -f "$BUILDDIR/fed.exe" || { echo "error: $BUILDDIR/fed.exe not found; run build first" >&2; exit 1; }

mkdir -p "$DISTDIR"

rm -f "$ARCHIVE"
zip -j "$ARCHIVE" "$BUILDDIR/fed.exe" readme.txt COPYING help.txt fed.syn

echo "Packaged: $ARCHIVE"
