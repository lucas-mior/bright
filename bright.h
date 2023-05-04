#define _POSIX_C_SOURCE 2
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <math.h>

typedef union String {
    char *p;
    char buf[12];
} String;

typedef struct Number {
    String string;
    int number;
} Number;

typedef struct Brightness {
    char file[PATH_MAX];
    Number absolute;
    int index;
} Brightness;

typedef enum Command {
    increase = '+',
    decrease = '-',
    help = 'h',
    print = 'p'
} Command;

#define NLEVELS 11

void send_signal(char *, int);
