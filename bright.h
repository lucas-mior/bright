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

#ifndef BRIGHT_H
#define BRIGHT_H

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

#ifndef INTEGERS
#define INTEGERS
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t usize;
typedef ssize_t isize;
#endif

typedef struct Number {
    char *string;
    int64 number;
} Number;

typedef struct Brightness {
    char file[PATH_MAX];
    int absolute;
    int index;
} Brightness;

enum {
    COMMAND_MORE = 0,
    COMMAND_LESS,
    COMMAND_FULL,
    COMMAND_HELP,
    COMMAND_PRINT,
};

struct Command {
    const char *shortname;
    const char *longname;
    const char *description;
};

static const struct Command commands[] = {
    [COMMAND_MORE] =  {"-m", "--more",  "more brightness"          },
    [COMMAND_LESS] =  {"-l", "--less",  "less brightness"          },
    [COMMAND_FULL] =  {"-f", "--full",  "full brightness"          },
    [COMMAND_PRINT] = {"-p", "--print", "print current brightness" },
    [COMMAND_HELP]  = {"-h", "--help",  "print this help message"  },
};

#define LENGTH(x) (int)(sizeof(x) / sizeof(x[0]))

#define NLEVELS 11
extern char *program;

#ifndef SNPRINTF
#define SNPRINTF(BUFFER, FORMAT, ...) \
    snprintf2(BUFFER, sizeof(BUFFER), FORMAT, __VA_ARGS__)
#endif

#endif
