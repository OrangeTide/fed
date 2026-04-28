#!/bin/sh
# Package the Linux ARM64 build into a .tar.gz archive.
#
# Environment variables:
#   BUILDDIR - build output directory  (default: build/linux-arm64)
#   DISTDIR  - package output directory (default: dist)
set -e

BUILDDIR="${BUILDDIR:-build/linux-arm64}"
DISTDIR="${DISTDIR:-dist}"
ARCHIVE="$DISTDIR/fed-linux-arm64.tar.gz"

test -x "$BUILDDIR/fed" || { echo "error: $BUILDDIR/fed not found; run build first" >&2; exit 1; }

mkdir -p "$DISTDIR"

cp readme.txt COPYING help.txt fed.syn "$BUILDDIR/"
tar czf "$ARCHIVE" -C "$BUILDDIR" fed readme.txt COPYING help.txt fed.syn

echo "Packaged: $ARCHIVE"
