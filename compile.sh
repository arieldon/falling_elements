#!/usr/bin/env sh

set -eux

BIN="sand"
COMPILER="clang"

CFLAGS="-std=c11 -D_DEFAULT_SOURCE"
WARNINGS="-Wall -Wextra -Wpedantic -Wshadow -Wno-unused-function"
LIBRARIES="-lX11"
FLAGS="$CFLAGS $WARNINGS $LIBRARIES"

if [ $# -ge 1 ] && [ "$1" = "--release" ]; then
	RELEASE="-O2 -march=native"
	FLAGS="$FLAGS $RELEASE"
	shift 1
else
	DEBUG="-DDEBUG -g3 -O0 -fno-omit-frame-pointer -fsanitize=undefined -fsanitize-undefined-trap-on-error"
	FLAGS="$FLAGS $DEBUG"
fi

if [ $# -ge 1 ]; then
	FLAGS="$FLAGS $@"
fi

$COMPILER main.c $FLAGS -o $BIN
