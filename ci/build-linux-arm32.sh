#!/bin/sh
# Build a static Linux ARM32 binary with ncurses.
# Runs natively on armv7l or cross-compiles using arm-linux-gnueabihf-gcc.
#
# Environment variables:
#   CROSS        - toolchain prefix        (default: arm-linux-gnueabihf- when cross-compiling)
#   CC           - target C compiler       (default: ${CROSS}gcc)
#   HOSTCC       - host C compiler         (default: gcc)
#   BUILDDIR     - output directory        (default: build/linux-arm32)
#   NCURSES_LIBS - ncurses link flags      (auto-detected via pkg-config)
set -e

if [ "$(uname -m)" = "armv7l" ]; then
	CROSS="${CROSS:-}"
else
	CROSS="${CROSS:-arm-linux-gnueabihf-}"
fi
CC="${CC:-${CROSS}gcc}"
HOSTCC="${HOSTCC:-gcc}"
BUILDDIR="${BUILDDIR:-build/linux-arm32}"

command -v "$CC" >/dev/null 2>&1 || { echo "error: $CC not found" >&2; exit 1; }
command -v "$HOSTCC" >/dev/null 2>&1 || { echo "error: $HOSTCC (host compiler) not found" >&2; exit 1; }

if ! echo '#include <curses.h>' | "$CC" -E -x c - >/dev/null 2>&1; then
	echo "error: ncurses headers not found for $CC" >&2
	exit 1
fi

if [ -z "$NCURSES_LIBS" ]; then
	PKG_CONFIG="${CROSS}pkg-config"
	if command -v "$PKG_CONFIG" >/dev/null 2>&1; then
		NCURSES_LIBS="$("$PKG_CONFIG" --static --libs ncurses 2>/dev/null)" || true
	fi
	NCURSES_LIBS="${NCURSES_LIBS:--lncurses}"
fi

mkdir -p "$BUILDDIR"

"$HOSTCC" -Wall -O2 -o "$BUILDDIR/makehelp" makehelp.c
"$BUILDDIR/makehelp" help.txt "$BUILDDIR/help.c"

CFLAGS="-DTARGET_CURSES -I. -Wall -O3 -fomit-frame-pointer"
SRCS="buffer.c config.c dialog.c disp.c fed.c gui.c help.c kill.c
      line.c menu.c misc.c search.c tetris.c util.c iocurses.c"

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

"$CC" -static -s -o "$BUILDDIR/fed" $OBJS $NCURSES_LIBS

echo "Built: $BUILDDIR/fed"
