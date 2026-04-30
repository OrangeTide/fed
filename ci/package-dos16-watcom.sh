#!/bin/sh
# Package the MS-DOS 16-bit (Open Watcom) build into a .zip archive.
#
# Environment variables:
#   BUILDDIR - build output directory  (default: build/dos16-watcom)
#   DISTDIR  - package output directory (default: dist)
set -e

BUILDDIR="${BUILDDIR:-build/dos16-watcom}"
DISTDIR="${DISTDIR:-dist}"
ARCHIVE="$DISTDIR/fed-dos16-watcom.zip"

test -f "$BUILDDIR/fed.exe" || { echo "error: $BUILDDIR/fed.exe not found; run build first" >&2; exit 1; }

mkdir -p "$DISTDIR"

rm -f "$ARCHIVE"
zip -j "$ARCHIVE" "$BUILDDIR/fed.exe" readme.txt COPYING help.txt fed.syn

echo "Packaged: $ARCHIVE"
