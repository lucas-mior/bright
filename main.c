// SPDX-License-Identifier: AGPL
// Copyright (c) 2026 Mior, Lucas

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

static bool get_bright(Brightness *);
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
        if (strequal(argv[1], commands[ic].shortname)
            || strequal(argv[1], commands[ic].longname)) {
            switch (ic) {
            case COMMAND_MORE:
            case COMMAND_LESS:
            case COMMAND_PRINT:
            case COMMAND_FULL:
                goto out;
            case COMMAND_HELP:
                main_usage(stdout);
            default:
                main_usage(stderr);
            }
        }
    }

    if (ic >= LENGTH(commands)) {
        main_usage(stderr);
    }

out:
    if (argc >= 3) {
        program_to_signal = argv[2];
    } else {
        program_to_signal = NULL;
    }

    SNPRINTF(max_bright.file, "%s/max_brightness", bright_directory);
    SNPRINTF(old_bright.file, "%s/brightness", bright_directory);
    SNPRINTF(new_bright.file, "%s/brightness", bright_directory);

    if (!get_bright(&max_bright)) {
        max_bright.absolute = 100000;
    }
    if (max_bright.absolute <= 60) {
        for (int32 i = 0; i < NLEVELS; i += 1) {
            levels[i] = i;
        }
    } else {
        int32 last;
        int32 second;
        int32 n;
        double m;
        double quotient;

        last = max_bright.absolute;
        second = last / 60;
        if (second <= 0) {
            second = 2;
        }

        n = NLEVELS - 2;
        m = (double)1 / (double)(n - 1);
        quotient = pow((double)last / (double)second, m);

        levels[0] = 0;
        levels[1] = 1;
        levels[2] = second;
        for (int32 i = 3; i < NLEVELS - 1; i += 1) {
            levels[i] = (int32)((double)levels[i - 1]*quotient);
        }
        levels[NLEVELS - 1] = last;
        levels[NLEVELS] = INT_MAX;
        if (DEBUGGING) {
            error("last:%d\n", last);
        }
    }

    if (!get_bright(&old_bright)) {
        old_bright.absolute = 100000;
    }

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
        main_usage(stderr);
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

bool
get_bright(Brightness *bright) {
    char buffer[32];
    int32 file;
    ssize_t r;

    if ((file = open(bright->file, O_RDONLY)) < 0) {
        error("Error opening file %s for getting brightness: %s\n",
              bright->file, strerror(errno));
        return false;
    }

    if ((r = read64(file, buffer, sizeof(buffer))) <= 0) {
        error("Error reading from file");
        if (r < 0) {
            error(": %s", strerror(errno));
        }
        error(".\n");
        XCLOSE(&file, bright->file);
        return false;
    }
    if (r >= SIZEOF(buffer)) {
        r = SIZEOF(buffer) - 1;
    }
    buffer[r] = '\0';

    bright->absolute = atoi(buffer);
    XCLOSE(&file, bright->file);
    return true;
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
