#!/usr/bin/env sh

set -eux

BIN="sand"
COMPILER="clang"

CFLAGS="-std=c11 -march=native -D_DEFAULT_SOURCE"
WARNINGS="-Wall -Wextra -Wshadow -Wconversion -Wdouble-promotion -Wno-unused-function -Wno-sign-conversion"
LIBRARIES="-lX11 -lGL"
FLAGS="$CFLAGS $WARNINGS $LIBRARIES"

if [ $# -ge 1 ] && [ "$1" = "--release" ]; then
	RELEASE="-O2"
	FLAGS="$FLAGS $RELEASE"
	shift 1
else
	DEBUG="-DDEBUG -g3 -O0 -fno-omit-frame-pointer -fsanitize=undefined -fsanitize-trap -fsanitize-undefined-trap-on-error"
	FLAGS="$FLAGS $DEBUG"
fi

if [ $# -ge 1 ]; then
	FLAGS="$FLAGS $@"
fi

$COMPILER main.c $FLAGS -o $BIN
