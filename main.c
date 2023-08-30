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

#include "bright.h"

bool between(int, int, int);
int find_index(int);
void create_levels(int);
void get_bright(Brightness *);
void save_new(Brightness *, Brightness *);
void main_usage(FILE *);

int main(int argc, char *argv[]) {
    char *program_to_signal = NULL;
    Command command;
    Brightness max_bright;
    Brightness old_bright;
    Brightness new_bright;

    switch (argc) {
    case 1: 
        command = print;
        break;
    case 3:
        program_to_signal = argv[2];
    case 2:
        command = argv[1][0];
        switch (command) {
        case increase:
        case decrease:
        case help:
        case print:
            break;
        default:
            main_usage(stderr);
        }
        break;
    default:
        main_usage(stderr);
    }

    snprintf(max_bright.file, sizeof(max_bright.file),
             "%s/max_brightness", bright_directory);
    snprintf(old_bright.file, sizeof(old_bright.file),
             "%s/brightness", bright_directory);
    snprintf(new_bright.file, sizeof(new_bright.file),
             "%s/brightness", bright_directory);

    get_bright(&max_bright);
    create_levels(max_bright.absolute);

    get_bright(&old_bright);
    old_bright.index = find_index(old_bright.absolute);

    new_bright.absolute = old_bright.absolute;
    new_bright.index = old_bright.index;

    switch (command) {
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
        main_usage(stdout);
        break;
    }

    save_new(&new_bright, &old_bright);
    printf("🔆 %i\n", new_bright.index);

    if (program_to_signal) {
        Number BRIGHT;
        if (!(BRIGHT.string = getenv("BRIGHT"))) {
            fprintf(stderr, "BRIGHT environment variable not set.\n");
            return 0;
        }
        if ((BRIGHT.number = atoi(BRIGHT.string)) < 10) {
            fprintf(stderr, "Invalid BRIGHT environment variable: %s.\n", BRIGHT.string);
            return 0;
        }

        send_signal(program_to_signal, BRIGHT.number);
    }

    return 0;
}

bool between(int a, int x, int b) {
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
    char buffer[12];

    if (!(file = fopen(bright->file, "r"))) {
        fprintf(stderr, "Can't open file for getting old bright.\n");
        return;
    }

    if (!fgets(buffer, sizeof(buffer), file)) {
        fprintf(stderr, "Can't read from file.\n");
        (void) fclose(file);
        return;
    }

    char *end_pointer = NULL;
    unsigned long aux = strtoul(buffer, &end_pointer, 10);
    if ((aux > INT_MAX) || (end_pointer == buffer)) {
        fprintf(stderr, "Invalid brightness read from file: %s\n", buffer);
        (void) fclose(file);
        exit(EXIT_FAILURE);
    }
    bright->absolute = (int) aux;

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

void main_usage(FILE *stream) {
    fprintf(stream, "usage: bright [%c%c%c%c] <s>\n", 
                    increase, decrease, help, print);
    fprintf(stream, "%c : decrease brightness\n", increase);
    fprintf(stream, "%c : increase brightness\n", decrease);
    fprintf(stream, "%c : show this help message\n", help);
    fprintf(stream, "%c : print current brightness\n", print);
    fprintf(stream, "if <s> is set, send $BRIGHT signal to <s>.\n");
    exit((int) (stream != stdout));
}
