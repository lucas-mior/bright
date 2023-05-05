#define _POSIX_C_SOURCE 2
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <math.h>

typedef struct Number {
    char *string;
    int number;
} Number;

typedef struct Brightness {
    char file[PATH_MAX];
    int absolute;
    int index;
} Brightness;

typedef enum Command {
    increase = '+',
    decrease = '-',
    help = 'h',
    print = 'p'
} Command;

#define NLEVELS 11
static int levels[NLEVELS];
static const char *bright_directory = "/sys/class/backlight/intel_backlight";

void send_signal(char *, int);
