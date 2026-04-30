#!/bin/sh
# Cross-compile an MS-DOS 32-bit (DOS32A) binary using Open Watcom.
#
# Toolchain requirements:
#   - Open Watcom v2 (github.com/open-watcom/open-watcom-v2)
#   - WATCOM env var pointing to the installation root
#
# Environment variables:
#   WATCOM   - Open Watcom installation   (required)
#   CC       - Watcom C compiler          (default: wcc386)
#   HOSTCC   - host C compiler            (default: gcc)
#   BUILDDIR - output directory           (default: build/dos32-watcom)
#
# Note: we deliberately omit -i=. to avoid the project's io.h shadowing
# Watcom's system <io.h>. wcc386 finds project headers via "" includes
# from the source file's directory.
set -e

CC="${CC:-wcc386}"
HOSTCC="${HOSTCC:-gcc}"
BUILDDIR="${BUILDDIR:-build/dos32-watcom}"

[ -n "$WATCOM" ] || { echo "error: WATCOM env var not set" >&2; exit 1; }
export INCLUDE="$WATCOM/h"

command -v "$CC" >/dev/null 2>&1 || { echo "error: $CC not found (is \$WATCOM/binl in PATH?)" >&2; exit 1; }
command -v wlink >/dev/null 2>&1 || { echo "error: wlink not found (is \$WATCOM/binl in PATH?)" >&2; exit 1; }
command -v "$HOSTCC" >/dev/null 2>&1 || { echo "error: $HOSTCC (host compiler) not found" >&2; exit 1; }

mkdir -p "$BUILDDIR"

"$HOSTCC" -Wall -O2 -o "$BUILDDIR/makehelp" makehelp.c
"$BUILDDIR/makehelp" help.txt "$BUILDDIR/help.c"

CFLAGS="-bt=dos -dTARGET_WATCOM -w4 -ox"
SRCS="buffer.c config.c dialog.c disp.c fed.c gui.c help.c kill.c
      line.c menu.c misc.c search.c tetris.c util.c iowatcom.c"

for src in $SRCS; do
	obj="$BUILDDIR/${src%.c}.o"
	case "$src" in
		help.c) "$CC" $CFLAGS -fo="$obj" "$BUILDDIR/help.c" ;;
		*)      "$CC" $CFLAGS -fo="$obj" "$src" ;;
	esac
done

OBJS=""
for src in $SRCS; do
	OBJS="${OBJS:+$OBJS,}$BUILDDIR/${src%.c}.o"
done

wlink system dos32a name "$BUILDDIR/fed.exe" file "$OBJS"

echo "Built: $BUILDDIR/fed.exe"
