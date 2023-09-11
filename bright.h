#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <linux/limits.h>

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

#pragma clang diagnostic ignored "-Wpadded"
typedef struct Number {
    char *string;
    int number;
} Number;

typedef struct Brightness {
    char file[PATH_MAX];
    int absolute;
    int index;
} Brightness;

enum {
    COMMAND_MORE = 0,
    COMMAND_LESS,
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
    [COMMAND_PRINT] = {"-p", "--print", "print current brightness" },
    [COMMAND_HELP]  = {"-h", "--help",  "print this help message"  },
};

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

#define NLEVELS 11
static int levels[NLEVELS];
static const char *bright_directory = "/sys/class/backlight/intel_backlight";

void send_signal(char *, int);
