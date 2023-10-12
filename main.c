/* This file is part of bright.
 * Copyright (C) 2022-2023 Lucas Mior

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bright.h"
#include "send_signal.h"

static bool between(int, int, int);
static int find_index(int);
static void create_levels(int);
static void get_bright(Brightness *);
static void save_new(Brightness *, Brightness *);
static void main_usage(FILE *) __attribute__((noreturn));

int main(int argc, char *argv[]) {
    bool spell_error = true;
    char *program_to_signal;
    Brightness max_bright;
    Brightness old_bright;
    Brightness new_bright;
    uint ic;

	int n, m, p;

    if ((argc <= 1) || (argc > 3))
        main_usage(stderr);

    for (ic = 0; ic < ARRAY_LENGTH(commands); ic += 1) {
        if (!strcmp(argv[1], commands[ic].shortname)
            || !strcmp(argv[1], commands[ic].longname)) {
            spell_error = false;
            switch (ic) {
            case COMMAND_MORE:
            case COMMAND_LESS:
            case COMMAND_PRINT:
                goto out;
            case COMMAND_HELP:
                main_usage(stdout);
            default:
                main_usage(stderr);
            }
        }
    }
    if (spell_error)
        main_usage(stderr);

    out:
    program_to_signal = argv[2];

    n = snprintf(max_bright.file, sizeof(max_bright.file),
                "%s/max_brightness", bright_directory);
    m = snprintf(old_bright.file, sizeof(old_bright.file),
                "%s/brightness", bright_directory);
    p = snprintf(new_bright.file, sizeof(new_bright.file),
                "%s/brightness", bright_directory);
	if (n < 0 || m < 0 || p < 0) {
		fprintf(stderr, "Error printing bright file names.\n");
		exit(EXIT_FAILURE);
	}

    get_bright(&max_bright);
    create_levels(max_bright.absolute);

    get_bright(&old_bright);
    old_bright.index = find_index(old_bright.absolute);

    new_bright.absolute = old_bright.absolute;
    new_bright.index = old_bright.index;

    switch (ic) {
    case COMMAND_PRINT:
        printf("🔆 %i", old_bright.index);
        exit(0);
    case COMMAND_LESS:
        if (0 < old_bright.index)
            new_bright.index -= 1;
        break;
    case COMMAND_MORE:
        if (old_bright.index < NLEVELS - 1)
            new_bright.index += 1;
        break;
    }

    save_new(&new_bright, &old_bright);

    if (program_to_signal) {
        Number BRIGHT;
        if (!(BRIGHT.string = getenv("DWMBLOCKS2_BRIGHT"))) {
            fprintf(stderr, "BRIGHT environment variable not set.\n");
            return 0;
        }
        if ((BRIGHT.number = (uint64) atol(BRIGHT.string)) < 10) {
            fprintf(stderr, "Invalid BRIGHT environment variable: %s.\n", BRIGHT.string);
            return 0;
        }

        send_signal(program_to_signal, (int) BRIGHT.number);
    }

    return 0;
}

bool between(int a, int x, int b) {
    return x < b && a <= x;
}

int find_index(int value) {
    int i = 0;

    while (i <= NLEVELS - 2) {
        if (between(levels[i], value, levels[i + 1]))
            return i;
        else
            i += 1;
    }

    return NLEVELS - 1;
}

void create_levels(int last) {
    int first = last / 60;
    int n = NLEVELS-2;
    double m = (double) 1 / (double) (n - 1);
    double quotient = pow((double) last / (double) first, m);

    int i;
    levels[i = 0] = 0;
    levels[i = 1] = 1;
    levels[i = 2] = first;
    for (i = 3; i < NLEVELS - 1; i += 1)
        levels[i] = (int) ((double) levels[i - 1] * quotient);
    levels[i = NLEVELS - 1] = last;

    return;
}

void get_bright(Brightness *bright) {
    FILE *file = NULL;
    char buffer[16];
    char *end_pointer = NULL;
    unsigned long aux;

    if (!(file = fopen(bright->file, "r"))) {
        fprintf(stderr, "Can't open file for getting old bright.\n");
        return;
    }

    if (!fgets(buffer, sizeof(buffer), file)) {
        fprintf(stderr, "Can't read from file.\n");
        (void) fclose(file);
        return;
    }

    aux = strtoul(buffer, &end_pointer, 10);
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
    fprintf(stream, "usage: %s COMMAND [program_to_signal]\n", "bright");
    fprintf(stream, "Available commands:\n");
    for (uint i = 0; i < ARRAY_LENGTH(commands); i += 1) {
        fprintf(stream, "%s | %-*s : %s\n",
                commands[i].shortname, 8, commands[i].longname, 
                commands[i].description);
    }
    exit(stream != stdout);
}
