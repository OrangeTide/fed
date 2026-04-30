#!/bin/sh
# Cross-compile an MS-DOS 32-bit binary using DJGPP.
#
# Toolchain requirements:
#   - DJGPP cross-compiler (e.g. from github.com/andrewwutw/build-djgpp)
#
# Environment variables:
#   CROSS    - toolchain prefix        (default: i586-pc-msdosdjgpp-)
#   CC       - target C compiler       (default: ${CROSS}gcc)
#   HOSTCC   - host C compiler         (default: gcc)
#   BUILDDIR - output directory        (default: build/dos32-djgpp)
set -e

CROSS="${CROSS:-i586-pc-msdosdjgpp-}"
CC="${CC:-${CROSS}gcc}"
HOSTCC="${HOSTCC:-gcc}"
BUILDDIR="${BUILDDIR:-build/dos32-djgpp}"

command -v "$CC" >/dev/null 2>&1 || { echo "error: $CC not found" >&2; exit 1; }
command -v "$HOSTCC" >/dev/null 2>&1 || { echo "error: $HOSTCC (host compiler) not found" >&2; exit 1; }

mkdir -p "$BUILDDIR"

"$HOSTCC" -Wall -O2 -o "$BUILDDIR/makehelp" makehelp.c
"$BUILDDIR/makehelp" help.txt "$BUILDDIR/help.c"

CFLAGS="-DTARGET_DJGPP -I. -Wall -O3 -fomit-frame-pointer"
SRCS="buffer.c config.c dialog.c disp.c fed.c gui.c help.c kill.c
      line.c menu.c misc.c search.c tetris.c util.c iodjgpp.c"

for src in $SRCS; do
	obj="$BUILDDIR/${src%.c}.o"
	case "$src" in
		help.c) "$CC" $CFLAGS -c "$BUILDDIR/help.c" -o "$obj" ;;
		*)      "$CC" $CFLAGS -c "$src" -o "$obj" ;;
	esac
done

OBJS=""
for src in $SRCS; do
	OBJS="$OBJS $BUILDDIR/${src%.c}.o"
done

"$CC" -s -o "$BUILDDIR/fed.exe" $OBJS

echo "Built: $BUILDDIR/fed.exe"
