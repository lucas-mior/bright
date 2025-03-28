/* This file is part of bright.
 * Copyright (C) 2024 Lucas Mior

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
#include "send_signal.c"

static inline void get_bright(Brightness *);
static void main_usage(FILE *) __attribute__((noreturn));
static char *snprintf2(char *, size_t, char *, ...);

char *program;
static int levels[NLEVELS];
static const char *bright_directory = "/sys/class/backlight/intel_backlight";

#define SNPRINTF(BUFFER, FORMAT, ...) do { \
    snprintf2(BUFFER, sizeof(BUFFER), FORMAT, __VA_ARGS__); \
} while (0)

int
main(int argc, char *argv[]) {
    bool spell_error = true;
    char *program_to_signal;
    Brightness max_bright;
    Brightness old_bright;
    Brightness new_bright;
    int ic;

    program = argv[0];


    if (argc <= 1) {
        ic = COMMAND_FULL;
        goto out;
    }
    if (argc > 3) {
        main_usage(stderr);
    }

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
    if (argc >= 3)
        program_to_signal = argv[2];
    else
        program_to_signal = NULL;

    SNPRINTF(max_bright.file, "%s/max_brightness", bright_directory);
    SNPRINTF(old_bright.file, "%s/brightness", bright_directory);
    SNPRINTF(new_bright.file, "%s/brightness", bright_directory);

    get_bright(&max_bright);
    {
        int last = max_bright.absolute;
        int first = last / 60;
        int n = NLEVELS - 2;
        double m = (double) 1 / (double) (n - 1);
        double quotient = pow((double) last / (double) first, m);

        int i;
        levels[i = 0] = 0;
        levels[i = 1] = 1;
        levels[i = 2] = first;
        for (i = 3; i < NLEVELS - 1; i += 1)
            levels[i] = (int) ((double) levels[i - 1] * quotient);
        levels[i = NLEVELS - 1] = last;
    }

    get_bright(&old_bright);

    for (int i = 0; i < NLEVELS; i += 1) {
        old_bright.index = i;
        if ((levels[i] <= old_bright.absolute)
             && (old_bright.absolute < levels[i + 1])) {
            break;
        }
    }

    new_bright.absolute = old_bright.absolute;
    new_bright.index = old_bright.index;

    switch (ic) {
    case COMMAND_PRINT:
        printf("🔆 %i", old_bright.index);
        exit(EXIT_SUCCESS);
    case COMMAND_LESS:
        if (0 < old_bright.index)
            new_bright.index -= 1;
        break;
    case COMMAND_MORE:
        if (old_bright.index < NLEVELS - 1)
            new_bright.index += 1;
        break;
    case COMMAND_FULL:
        new_bright.index = NLEVELS - 1;
        break;
    default:
        fprintf(stderr, "Unexpected value: %d\n", ic); 
        exit(EXIT_FAILURE);
    }

    {
        FILE *save;

        if (!(save = fopen(new_bright.file, "w"))) {
            error("Can't open file for setting current brightness: %s\n",
                  strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (fprintf(save, "%d\n", levels[new_bright.index]) < 0) {
            error("Can't write to file.\n");
            new_bright.index = old_bright.index;
            fclose(save);
            exit(EXIT_FAILURE);
        }
        fclose(save);
    }

    if (program_to_signal) {
        Number DWMBLOCKS2_BRIGHT;

        if (!(DWMBLOCKS2_BRIGHT.string = getenv("DWMBLOCKS2_BRIGHT"))) {
            error("%s environment variable not set.\n", "DWMBLOCKS2_BRIGHT");
            exit(EXIT_FAILURE);
        }
        if ((DWMBLOCKS2_BRIGHT.number = atol(DWMBLOCKS2_BRIGHT.string)) < 10) {
            error("Invalid BRIGHT environment variable: %s.\n",
                   DWMBLOCKS2_BRIGHT.string);
            exit(EXIT_FAILURE);
        }

        send_signal(program_to_signal,
                    (int)(SIGRTMIN + DWMBLOCKS2_BRIGHT.number));
    }

    exit(EXIT_SUCCESS);
}

void
get_bright(Brightness *bright) {
    char buffer[16];
    int file;
    ssize_t r;

    if ((file = open(bright->file, O_RDONLY)) < 0) {
        error("Can't open file for getting old bright: %s\n", strerror(errno));
        return;
    }

    if ((r = read(file, buffer, sizeof(buffer) - 1)) <= 0) {
        error("Can't read from file.");
        if (r < 0)
            error(": %s", strerror(errno));
        error(".\n");
        close(file);
        return;
    }
    buffer[r] = '\0';

    bright->absolute = atoi(buffer);
    close(file);
    return;
}

void
main_usage(FILE *stream) {
    fprintf(stream, "usage: %s COMMAND [program_to_signal]\n", "bright");
    fprintf(stream, "Available commands:\n");
    for (uint i = 0; i < ARRAY_LENGTH(commands); i += 1) {
        fprintf(stream, "%s | %-*s : %s\n",
                commands[i].shortname, 8, commands[i].longname, 
                commands[i].description);
    }
    exit(stream != stdout);
}

char *
snprintf2(char *buffer, size_t size, char *format, ...) {
    int n;
    va_list args;

    va_start(args, format);
    n = vsnprintf(buffer, size, format, args);
    va_end(args);

    if (n > (int)size) {
        error("Error in snprintf: Too large string.\n");
        exit(EXIT_FAILURE);
    }
    if (n <= 0) {
        error("Error in snprintf.\n");
        exit(EXIT_FAILURE);
    }
    return buffer;
}

void
error(char *format, ...) {
    int n;
    va_list args;
    char buffer[BUFSIZ];

    va_start(args, format);
    n = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);

    if (n < 0) {
        fprintf(stderr, "Error in vsnprintf()\n");
        exit(EXIT_FAILURE);
    }

    buffer[n] = '\0';
    write(STDERR_FILENO, buffer, (usize) n);

#ifdef DEBUGGING
    switch (fork()) {
    case -1:
        fprintf(stderr, "Error forking: %s\n", strerror(errno));
        break;
    case 0:
        execlp("dunstify", "dunstify", "-u", "critical", program, buffer, NULL);
        fprintf(stderr, "Error trying to exec dunstify.\n");
        break;
    default:
        break;
    }
    exit(EXIT_FAILURE);
#endif
}
