#!/bin/sh
# Package the MS-DOS 32-bit (DJGPP) build into a .zip archive.
#
# Environment variables:
#   BUILDDIR - build output directory  (default: build/dos32-djgpp)
#   DISTDIR  - package output directory (default: dist)
set -e

BUILDDIR="${BUILDDIR:-build/dos32-djgpp}"
DISTDIR="${DISTDIR:-dist}"
ARCHIVE="$DISTDIR/fed-dos32-djgpp.zip"

test -f "$BUILDDIR/fed.exe" || { echo "error: $BUILDDIR/fed.exe not found; run build first" >&2; exit 1; }

mkdir -p "$DISTDIR"

rm -f "$ARCHIVE"
zip -j "$ARCHIVE" "$BUILDDIR/fed.exe" readme.txt COPYING help.txt fed.syn

echo "Packaged: $ARCHIVE"
