#!/bin/sh

testing () {
    for src in *.c; do
        [ "$src" = "main.c" ] && continue
        printf "Testing $src...\n"

        flags="$(awk '/flags:/ { $1=$2=""; print $0 }' "$src")"
        set -x
        if $CC $CFLAGS -D TESTING_THIS_FILE=1 $src -o $src.exe $flags; then
            ./$src.exe
        else
            printf "Failed to compile ${RED} $src ${RES}, is main() defined?\n"
        fi

        set +x 
    done
    rm *.exe
}

target="${1:-build}"
PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"

SRC=$(ls *.c)

CFLAGS="$CFLAGS -std=c99 -D_DEFAULT_SOURCE "
CFLAGS="$CFLAGS -Wextra -Wall -Wno-disabled-macro-expansion -Wno-unused-macros"
LDFLAGS="$LDFLAGS -lm "

CC=${CC:-cc}
if [ $CC = "clang" ]; then
    CFLAGS="$CFLAGS -Weverything "
    CFLAGS="$CFLAGS -Wno-unsafe-buffer-usage -Wno-format-nonliteral "
fi

if [ "$target" = "debug" ]; then
    CFLAGS="$CFLAGS -g -fsanitize=undefined "
else
    CFLAGS="$CFLAGS -O2 -flto "
fi

case "$target" in
    "uninstall")
        set -x
        rm -f ${DESTDIR}${PREFIX}/bin/bright
        rm -f ${DESTDIR}${PREFIX}/man/man1/bright.1
        ;;
    "test")
        testing
        ;;
    "install")
        set -x
        install -Dm755 bright ${DESTDIR}${PREFIX}/bin/bright
        install -Dm644 bright.1 ${DESTDIR}${PREFIX}/man/man1/bright.1
        ;;
    "build"|"debug")
        ctags --kinds-C=+l *.h *.c 2> /dev/null || true
        vtags.sed tags > .tags.vim 2> /dev/null || true
        set -x
        $CC $CFLAGS -o bright main.c $LDFLAGS
        ;;
    *)
        echo "usage: $0 [ uninstall / test / install / build / debug ]"
        ;;
esac
