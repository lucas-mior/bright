// SPDX-License-Identifier: AGPL
// Copyright (c) 2026 Mior, Lucas

#include <errno.h>
#include <math.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/types.h>
#include <linux/limits.h>

#define CBASE_IMPLEMENT
#include "cbase.h"

typedef struct Brightness {
    char file[PATH_MAX];
    int32 absolute;
    int32 index;
} Brightness;

enum {
    COMMAND_MORE = 0,
    COMMAND_LESS,
    COMMAND_FULL,
    COMMAND_HELP,
    COMMAND_PRINT,
};

struct BrightCommand {
    const char *shortname;
    const char *longname;
    const char *description;
};

static const struct BrightCommand commands[] = {
    [COMMAND_MORE] =  {"-m", "--more",  "more brightness"},
    [COMMAND_LESS] =  {"-l", "--less",  "less brightness"},
    [COMMAND_FULL] =  {"-f", "--full",  "full brightness"},
    [COMMAND_PRINT] = {"-p", "--print", "print current brightness"},
    [COMMAND_HELP] =  {"-h", "--help",  "print this help message"},
};

#define NLEVELS 11

#if !defined(SNPRINTF)
#define SNPRINTF(BUFFER, FORMAT, ...) \
    snprintf2(BUFFER, sizeof(BUFFER), FORMAT, __VA_ARGS__)
#endif

static void get_bright(Brightness *);
static void main_usage(FILE *) __attribute__((noreturn));

static int32 levels[NLEVELS + 1];
static const char *bright_directory = "/sys/class/backlight/intel_backlight";

int32
main(int32 argc, char *argv[]) {
    char *program_to_signal;
    Brightness max_bright;
    Brightness old_bright;
    Brightness new_bright;
    int32 ic;

    program = argv[0];

    if (argc <= 1) {
        ic = COMMAND_FULL;
        goto out;
    }
    if (argc > 3) {
        main_usage(stderr);
    }

    for (ic = 0; ic < LENGTH(commands); ic += 1) {
        if (!strcmp(argv[1], commands[ic].shortname)
            || !strcmp(argv[1], commands[ic].longname)) {
            switch (ic) {
            case COMMAND_MORE:
            case COMMAND_LESS:
            case COMMAND_PRINT:
                goto out;
            case COMMAND_HELP:
                main_usage(stdout);
            // TODO: COMMAND_FULL is listed as a valid command, but this
            // parser treats `bright --full` as invalid.
            default:
                main_usage(stderr);
            }
        }
    }

    // TODO: Unknown commands fall through with ic == LENGTH(commands), so
    // the program reads brightness before reporting "Unexpected value".
out:
    if (argc >= 3) {
        program_to_signal = argv[2];
    } else {
        program_to_signal = NULL;
    }

    SNPRINTF(max_bright.file, "%s/max_brightness", bright_directory);
    SNPRINTF(old_bright.file, "%s/brightness", bright_directory);
    SNPRINTF(new_bright.file, "%s/brightness", bright_directory);

    get_bright(&max_bright);
    {
        int32 last;
        int32 first;
        int32 n;
        double m;
        double quotient;

        last = max_bright.absolute;
        first = last / 60;
        if (first <= 0) {
            first = 1;
        }

        // TODO: If max_brightness is 60..119, integer truncation keeps
        // many adjacent levels equal, so --less/--more can stop changing it.
        n = NLEVELS - 2;
        m = (double)1 / (double)(n - 1);
        quotient = pow((double)last / (double)first, m);

        levels[0] = 0;
        levels[1] = 1;
        levels[2] = first;
        for (int32 i = 3; i < NLEVELS - 1; i += 1) {
            levels[i] = (int32)((double)levels[i - 1]*quotient);
        }
        levels[NLEVELS - 1] = last;
        levels[NLEVELS] = INT_MAX;
        if (DEBUGGING) {
            error("last:%d\n", last);
        }
    }

    get_bright(&old_bright);

    for (int32 i = 0; i < NLEVELS; i += 1) {
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
        if (0 < old_bright.index) {
            new_bright.index -= 1;
        }
        break;
    case COMMAND_MORE:
        if (old_bright.index < NLEVELS - 1) {
            new_bright.index += 1;
        }
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
            error("Error writing to file: %s.\n", strerror(errno));
            new_bright.index = old_bright.index;
            XFCLOSE(save, new_bright.file);
            exit(EXIT_FAILURE);
        }
        XFCLOSE(save, new_bright.file);
    }

    if (program_to_signal) {
        char *DWMBLOCKS2_BRIGHT;
        int32 number;

        GETENV(DWMBLOCKS2_BRIGHT);
        if (DWMBLOCKS2_BRIGHT == NULL) {
            exit(EXIT_FAILURE);
        }
        if ((number = atoi(DWMBLOCKS2_BRIGHT)) < 10) {
            error("Invalid environment variable: %s = %s.\n",
                  QUOTE(DWMBLOCKS2_BRIGHT), DWMBLOCKS2_BRIGHT);
            exit(EXIT_FAILURE);
        }

        send_signal(program_to_signal, SIGRTMIN + number);
    }

    exit(EXIT_SUCCESS);
}

void
get_bright(Brightness *bright) {
    char buffer[16];
    int32 file;
    ssize_t r;

    if ((file = open(bright->file, O_RDONLY)) < 0) {
        error("Can't open file %s for getting brightness: %s\n",
              bright->file, strerror(errno));
        // TODO: Callers use bright->absolute after this returns, so this
        // failure path leaves them reading an uninitialized value.
        return;
    }

    if ((r = read64(file, buffer, sizeof(buffer))) <= 0) {
        error("Can't read from file.");
        if (r < 0) {
            error(": %s", strerror(errno));
        }
        error(".\n");
        XCLOSE(&file, bright->file);
        // TODO: Callers use bright->absolute after this returns, so this
        // failure path leaves them reading an uninitialized value.
        return;
    }
    // TODO: read64() can fill all 16 bytes, so buffer[r] writes past the
    // end when r == sizeof(buffer).
    buffer[r] = '\0';

    bright->absolute = atoi(buffer);
    close(file);
    return;
}

void
main_usage(FILE *stream) {
    fprintf(stream, "usage: %s COMMAND [program_to_signal]\n", "bright");
    fprintf(stream, "Available commands:\n");
    for (uint i = 0; i < LENGTH(commands); i += 1) {
        fprintf(stream, "%s | %-*s : %s\n", commands[i].shortname, 8,
                commands[i].longname, commands[i].description);
    }
    exit(stream != stdout);
}
