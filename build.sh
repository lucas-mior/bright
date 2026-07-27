#!/bin/sh

# shellcheck disable=SC2086

testing () {
    for src in *.c; do
        [ "$src" = "$main" ] && continue
        printf "Testing $src...\n"

        name="$(echo "$src" | sed 's/\.c//g')"
        flags="$(awk '/flags:/ { $1=$2=""; print $0 }' "$src")"
        set -x
        if $CC $CFLAGS -D TESTING_$name=1 $src -o $src.exe $flags; then
            ./$src.exe
        else
            printf "Failed to compile ${RED} $src ${RES}, is main() defined?\n"
        fi

        set +x
    done
    rm -- *.exe
}

target="${1:-build}"
PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"

main="main.c"
program=$(basename "$(realpath "$(dirname "$0")")")

CFLAGS="$CFLAGS -std=c11 -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=700"
CFLAGS="$CFLAGS -Wextra -Wall"
CFLAGS="$CFLAGS -Wno-disabled-macro-expansion"
CFLAGS="$CFLAGS -Wno-unused-macros"
CFLAGS="$CFLAGS -Wno-unused-function"
CFLAGS="$CFLAGS -Wno-constant-logical-operand"
CFLAGS="$CFLAGS -Wno-gnu-union-cast"
LDFLAGS="$LDFLAGS -lm "

dir="$(readlink -f "$(dirname "$0")")"
cbase="cbase"
CPPFLAGS="$CPPFLAGS -I "$dir/$cbase""

CC=${CC:-cc}

if [ $CC = "clang" ]; then
    CFLAGS="$CFLAGS -Weverything"
    CFLAGS="$CFLAGS -Wno-unsafe-buffer-usage"
    CFLAGS="$CFLAGS -Wno-format-nonliteral"
    CFLAGS="$CFLAGS -Wno-format-pedantic"
    CFLAGS="$CFLAGS -Wno-pre-c11-compat"
    CFLAGS="$CFLAGS -Wno-c++-keyword"
    CFLAGS="$CFLAGS -Wno-covered-switch-default"
    CFLAGS="$CFLAGS -Wno-implicit-void-ptr-cast"
    CFLAGS="$CFLAGS -Wno-cast-qual"
    CFLAGS="$CFLAGS -Wno-float-equal"
    CFLAGS="$CFLAGS -Wno-cast-function-type-strict"
    CFLAGS="$CFLAGS -Wno-assign-enum"
    CFLAGS="$CFLAGS -Wno-implicit-int-enum-cast"
fi

if [ "$target" = "debug" ]; then
    CFLAGS="$CFLAGS -g -fsanitize=undefined -DDEBUGGING=1"
else
    CFLAGS="$CFLAGS -O2 -flto "
fi

case "$target" in
"uninstall")
    set -x
    rm -f ${DESTDIR}${PREFIX}/bin/${program}
    rm -f ${DESTDIR}${PREFIX}/man/man1/${program}.1
    ;;
"check")
    CC=gcc CFLAGS="-fanalyzer -fdiagnostics-color=never" "$0" build
    CFLAGS="--analyze -Xanalyzer -analyzer-output=text"
    CFLAGS="$CFLAGS -Xanalyzer -analyzer-werror"
    CFLAGS="$CFLAGS -Xanalyzer -analyzer-opt-analyze-headers"
    CFLAGS="$CFLAGS -Wno-unused-command-line-argument"
    CFLAGS="$CFLAGS -fno-color-diagnostics"
    CC=clang CFLAGS="$CFLAGS" "$0" build
    ;;
"test")
    testing
    ;;
"install")
    [ ! -f $program ] && $0 build
    set -x
    install -Dm755 ${program} ${DESTDIR}${PREFIX}/bin/${program}
    install -Dm644 ${program}.1 ${DESTDIR}${PREFIX}/man/man1/${program}.1
    ;;
"build"|"debug")
    ctags --kinds-C=+l -- *.h *.c 2> /dev/null || true
    vtags.sed tags > .tags.vim 2> /dev/null || true
    set -x
    $CC $CPPFLAGS $CFLAGS -o ${program} $main $LDFLAGS
    ;;
*)
    echo "usage: $0 [ check / uninstall / test / install / build / debug ]"
    ;;
esac
