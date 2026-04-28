#!/bin/sh
# Cross-compile a Windows x86-64 binary using MinGW.
#
# Toolchain requirements:
#   - x86_64-w64-mingw32-gcc (and optionally windres for icons)
#
# Environment variables:
#   CROSS    - toolchain prefix        (default: x86_64-w64-mingw32-)
#   CC       - target C compiler       (default: ${CROSS}gcc)
#   WINDRES  - resource compiler       (default: ${CROSS}windres)
#   HOSTCC   - host C compiler         (default: gcc)
#   BUILDDIR - output directory        (default: build/win64)
set -e

CROSS="${CROSS:-x86_64-w64-mingw32-}"
CC="${CC:-${CROSS}gcc}"
WINDRES="${WINDRES:-${CROSS}windres}"
HOSTCC="${HOSTCC:-gcc}"
BUILDDIR="${BUILDDIR:-build/win64}"

command -v "$CC" >/dev/null 2>&1 || { echo "error: $CC not found" >&2; exit 1; }
command -v "$HOSTCC" >/dev/null 2>&1 || { echo "error: $HOSTCC (host compiler) not found" >&2; exit 1; }

mkdir -p "$BUILDDIR"

"$HOSTCC" -Wall -O2 -o "$BUILDDIR/makehelp" makehelp.c
"$BUILDDIR/makehelp" help.txt "$BUILDDIR/help.c"

CFLAGS="-DTARGET_WIN -iquote . -Wall -O3 -fomit-frame-pointer"
SRCS="buffer.c config.c dialog.c disp.c fed.c gui.c help.c kill.c
      line.c menu.c misc.c search.c tetris.c util.c iowin.c"

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

if command -v "$WINDRES" >/dev/null 2>&1 && [ -f fed.rc ]; then
	"$WINDRES" fed.rc -o "$BUILDDIR/fed_res.o"
	OBJS="$OBJS $BUILDDIR/fed_res.o"
fi

"$CC" -s -mwindows -o "$BUILDDIR/fed.exe" $OBJS \
	-luser32 -lgdi32 -lshell32 -lwinmm -ladvapi32

echo "Built: $BUILDDIR/fed.exe"
