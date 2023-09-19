#!/usr/bin/env sh

set -eux

BIN="falling_elements"

COMPILER="clang"
LIBRARIES="-lX11 -lGL"
CFLAGS="-std=c11 -march=native -D_DEFAULT_SOURCE"
WARNINGS="-Wall -Wextra -Wshadow -Wconversion -Wdouble-promotion -Wno-unused-function -Wno-sign-conversion -Wno-string-conversion"

if [ $# -ge 1 ] && [ "$1" = "--windows" ]; then
	COMPILER="x86_64-w64-mingw32-gcc"
	LIBRARIES="-mwindows -lgdi32 -lopengl32"
	WARNINGS="$WARNINGS -Wno-parentheses"
	shift 1
fi

FLAGS="$CFLAGS $WARNINGS $LIBRARIES"

if [ $# -ge 1 ] && [ "$1" = "--release" ]; then
	RELEASE="-O2"
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
