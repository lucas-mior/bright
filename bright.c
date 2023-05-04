/* This file is part of bright. */
/* Copyright (C) 2022 Lucas Mior */

/* This program is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or */
/* (at your option) any later version. */

/* This program is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#define _POSIX_C_SOURCE 2
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <math.h>

#include "send_signal.h"

typedef union Number {
    char *string;
    int number;
} Number;

typedef struct Brightness {
    char file[PATH_MAX];
    Number absolute;
    int index;
} Brightness;


#define NLEVELS 11
static int levels[NLEVELS];
static const char *bright_dir = "/sys/class/backlight/intel_backlight";

inline bool between(int a, int x, int b) {
    return x < b && a <= x;
}

int find_index(int value) {
    int i = 0;

    while (i <= NLEVELS - 2) {
        if (between(levels[i], value, levels[i+1]))
            return i;
        else
            i++;
    }

    return NLEVELS - 1;
}

void create_levels(int last) {
    int first = last / 60;
    int n = NLEVELS-2;
    double m = (double) 1 / (double) (n-1);
    double quotient = pow((double) last / (double) first, m);

    int i;
    levels[i = 0] = 0;
    levels[i = 1] = 1;
    levels[i = 2] = first;
    for (i = 3; i < NLEVELS-1; i += 1)
        levels[i] = (double) levels[i-1] * quotient;
    levels[i = NLEVELS-1] = last;

    return;
}

void get_bright(Brightness *bright) {
    FILE *file = NULL;

    if (!(file = fopen(bright->file, "r"))) {
        fprintf(stderr, "Can't open file for getting old bright.\n");
        return;
    }

    if (!fgets(bright->absolute.string, 9, file)) {
        fprintf(stderr, "Can't read from file.\n");
        (void) fclose(file);
        return;
    }

    char *end_pointer = NULL;
    unsigned long aux = strtoul(bright->absolute.string, &end_pointer, 10);
    if ((aux > INT_MAX) || (end_pointer == bright->absolute.string)) {
        fprintf(stderr, "Invalid brightness read from file: "
                        "string: %s"
                        "integer: %lu\n", bright->absolute.string, aux);
        (void) fclose(file);
        exit(1);
    }
    bright->absolute.number = (int) aux;

    (void) fclose(file);
    return;
}

void save_new(Brightness *new_bright, Brightness *old_bright) {
    FILE *save;

    if (!(save = fopen(new_bright->file, "w"))) {
        fprintf(stderr, "Can't open file for setting current brightness.\n");
        return;
    }
    if (fprintf(save, "%i\n", levels[new_bright->index]) < 0) {
        fprintf(stderr, "Can't write to file.\n");
        new_bright->index = old_bright->index;
        (void) fclose(save);
        return;
    }
    (void) fclose(save);
    return;
}

typedef enum Command {
    increase = '+',
    decrease = '-',
    help = 'h',
    print = 'p'
} Command;

void usage(FILE *stream) {
    fprintf(stream, "usage: bright [%c%c%c%c] <s>\n", 
                    increase, decrease, help, print);
    fprintf(stream, "%c : decrease brightness\n", increase);
    fprintf(stream, "%c : increase brightness\n", decrease);
    fprintf(stream, "%c : show this help message\n", help);
    fprintf(stream, "%c : print current brightness\n", print);
    fprintf(stream, "if <s> is set, send $BRIGHT signal to <s>.\n");
    exit((int) (stream != stdout));
    return;
}

int main(int argc, char *argv[]) {
    char *prog_to_sig = NULL;
    Command c;
    switch (argc) {
    case 1: 
        c = print;
        break;
    case 3:
        prog_to_sig = argv[2];
        __attribute__((fallthrough));
    case 2:
        c = argv[1][0];
        switch (c) {
        case increase: __attribute__((fallthrough));
        case decrease: __attribute__((fallthrough));
        case help: __attribute__((fallthrough));
        case print:
            break;
        default:
            usage(stderr);
            break;
        }
        break;
    default:
        usage(stderr);
        return 1;
    }

    Brightness max_bright, old_bright, new_bright;

    snprintf(max_bright.file, sizeof(max_bright.file), "%s/max_brightness", bright_dir);
    snprintf(old_bright.file, sizeof(old_bright.file), "%s/brightness", bright_dir);
    snprintf(new_bright.file, sizeof(new_bright.file), "%s/brightness", bright_dir);

    get_bright(&max_bright);
    create_levels(max_bright.absolute.number);

    get_bright(&old_bright);
    old_bright.index = find_index(old_bright.absolute.number);

    new_bright.absolute.number = old_bright.absolute.number;
    new_bright.index = old_bright.index;

    switch (c) {
    case print:
        printf("🔆 %i", old_bright.index);
        exit(0);
    case decrease:
        if (0 < old_bright.index)
            new_bright.index -= 1;
        break;
    case increase:
        if (old_bright.index < NLEVELS - 1)
            new_bright.index += 1;
        break;
    case help:
        usage(stdout);
        break;
    }

    save_new(&new_bright, &old_bright);
    printf("🔆 %i\n", new_bright.index);

    if (prog_to_sig) {
        Number bright;
        if (!(bright.string = getenv("BRIGHT"))) {
            fprintf(stderr, "BRIGHT environment variable not set.\n");
            return 0;
        }
        if ((bright.number = atoi(bright.string)) < 10) {
            fprintf(stderr, "Invalid BRIGHT environment variable.\n");
            return 0;
        }

        send_signal(prog_to_sig, bright.number);
    }

    return 0;
}
