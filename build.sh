#!/bin/sh -e

# shellcheck disable=SC2086

dir=$(dirname "$(readlink -f "$0")")
# shellcheck source=/dev/null
. "$dir/cbase/common.sh"

cd "$dir" || exit
program=$(get_program "$0")
script=$(basename "$0")
target="${1:-debug}"

printf "\n${script} ${RED}${1:-} ${2:-}$RES\n"

PREFIX="${PREFIX:-/usr/local}"
DESTDIR="${DESTDIR:-/}"

exe="bin/$program"
mkdir -p "$(dirname "$exe")"

CC=$(get_compiler "$target")

CPPFLAGS="$CPPFLAGS -I$dir/cbase"
CPPFLAGS="$CPPFLAGS -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=700"

CFLAGS="$CFLAGS -std=c11"
CFLAGS="$CFLAGS -Wfatal-errors"
CFLAGS="$CFLAGS -Wextra -Wall"
CFLAGS="$CFLAGS -Werror=all -Werror=extra"
CFLAGS="$CFLAGS -Werror"  # Only uncomment occasionally, keep this line

if [ "$CC" = "clang" ]; then
    CFLAGS="$CFLAGS -Weverything"
    CFLAGS="$CFLAGS -Wno-assign-enum"
    CFLAGS="$CFLAGS -Wno-c++-keyword"
    CFLAGS="$CFLAGS -Wno-cast-qual"
    CFLAGS="$CFLAGS -Wno-constant-logical-operand"
    CFLAGS="$CFLAGS -Wno-covered-switch-default"
    CFLAGS="$CFLAGS -Wno-disabled-macro-expansion"
    CFLAGS="$CFLAGS -Wno-float-equal"
    CFLAGS="$CFLAGS -Wno-format-nonliteral"
    CFLAGS="$CFLAGS -Wno-ignored-attributes"
    CFLAGS="$CFLAGS -Wno-implicit-int-enum-cast"
    CFLAGS="$CFLAGS -Wno-implicit-void-ptr-cast"
    CFLAGS="$CFLAGS -Wno-pre-c11-compat"
    CFLAGS="$CFLAGS -Wno-unsafe-buffer-usage"
    CFLAGS="$CFLAGS -Wno-unused-macros"
    CFLAGS="$CFLAGS -Wno-used-but-marked-unused"
fi

LDFLAGS="$LDFLAGS -lm"

OS=$(uname -a)
GNUSOURCE=
if echo "$OS" | grep -q "Linux"; then
    if echo "$OS" | grep -q "GNU"; then
        GNUSOURCE="-D_GNU_SOURCE"
    fi
fi

case "$target" in
debug)
    CFLAGS="$CFLAGS -g3 -O0 -fsanitize=undefined"
    CPPFLAGS="$CPPFLAGS $GNUSOURCE -DDEBUGGING=1"
    exe="bin/${program}_debug"
    ;;
test)
    CFLAGS="$CFLAGS -g3 -O0 $GNUSOURCE -DDEBUGGING=1"
    CFLAGS="$CFLAGS -fsanitize=undefined"
    ;;
build)
    CFLAGS="$CFLAGS $GNUSOURCE -O2 -flto -march=native -ftree-vectorize"
    ;;
fast_feedback)
    CFLAGS="$CFLAGS $GNUSOURCE"
    ;;
*)
    CFLAGS="$CFLAGS -O2"
    ;;
esac

case "$target" in
fast_feedback)
    build_tags
    trace_on
    $CC $CPPFLAGS $CFLAGS -o "$exe" main.c $LDFLAGS && LC_ALL=C "$exe"
    trace_off
    ;;
uninstall)
    trace_on
    rm -f "${DESTDIR}${PREFIX}/bin/${program}"
    uninstall_opt "${program}.1" "${DESTDIR}${PREFIX}/man/man1/${program}.1"
    trace_off
    ;;
check)
    CC=gcc CFLAGS="-fanalyzer -fdiagnostics-color=never" "$0" build
    CFLAGS="--analyze -Xanalyzer -analyzer-output=text"
    CFLAGS="$CFLAGS -Xanalyzer -analyzer-werror"
    CFLAGS="$CFLAGS -Xanalyzer -analyzer-opt-analyze-headers"
    CFLAGS="$CFLAGS -Wno-unused-command-line-argument"
    CFLAGS="$CFLAGS -fno-color-diagnostics"
    CC=clang CFLAGS="$CFLAGS" "$0" build
    exit
    ;;
test)
    find . -maxdepth 1 -name "*.c" | sort | while read -r src; do
        name=$(basename "$src")
        [ "$name" = "main.c" ] && continue
        printf "Testing %s...\n" "$src"

        name=$(echo "$name" | sed 's/\.c//g')
        flags=$(awk '/flags:/ { $1=$2=""; print $0 }' "$src")
        test_exe="/tmp/${name}_test"

        trace_on
        if $CC $CPPFLAGS $CFLAGS \
            -DCBASE_IMPLEMENT -DTESTING_$name=1 "$src" \
              -o "$test_exe" $LDFLAGS $flags; then
            "$test_exe"
        else
            error "Failed to compile %s, is main() defined?\n" "$src"
        fi
        trace_off
    done
    ;;
install)
    if [ ! -f "$exe" ]; then
        "$0" build
    fi
    trace_on
    install -Dm755 "$exe" "${DESTDIR}${PREFIX}/bin/${program}"
    install_opt -Dm644 "${program}.1" "${DESTDIR}${PREFIX}/man/man1/${program}.1"
    trace_off
    ;;
*)
    build_tags

    trace_on
    $CC $CPPFLAGS $CFLAGS -o "$exe" main.c $LDFLAGS
    trace_off
    ;;
esac
