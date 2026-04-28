#!/bin/sh
# Build a static Linux x86-64 binary using musl libc + ncurses.
#
# Toolchain requirements:
#   - C compiler targeting musl (gcc on Alpine, or musl-gcc on glibc distros)
#   - ncurses static library and headers available to the compiler
#
# Environment variables:
#   CC           - target C compiler      (default: gcc)
#   HOSTCC       - host C compiler         (default: gcc)
#   BUILDDIR     - output directory        (default: build/linux-x86_64)
#   NCURSES_LIBS - ncurses link flags      (auto-detected via pkg-config)
set -e

CC="${CC:-gcc}"
HOSTCC="${HOSTCC:-gcc}"
BUILDDIR="${BUILDDIR:-build/linux-x86_64}"

command -v "$CC" >/dev/null 2>&1 || { echo "error: $CC not found" >&2; exit 1; }
command -v "$HOSTCC" >/dev/null 2>&1 || { echo "error: $HOSTCC (host compiler) not found" >&2; exit 1; }

if ! echo '#include <curses.h>' | "$CC" -E -x c - >/dev/null 2>&1; then
	echo "error: ncurses headers not found for $CC" >&2
	exit 1
fi

if [ -z "$NCURSES_LIBS" ]; then
	if command -v pkg-config >/dev/null 2>&1; then
		NCURSES_LIBS="$(pkg-config --static --libs ncurses 2>/dev/null)" || true
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
