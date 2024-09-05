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

#ifndef SEND_SIGNAL_C
#define SEND_SIGNAL_C

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include "bright.h"

void
send_signal(char *executable, int signal_number) {
    DIR *processes;
    struct dirent *process;

    if (!(processes = opendir("/proc"))) {
        error("Error opening /proc: %s\n", strerror(errno));
        return;
    }

    while ((process = readdir(processes))) {
        static char buffer[256];
        static char command[256];
        int n;
        int pid;
        int cmdline;

        if ((pid = atoi(process->d_name)) <= 0)
            continue;

        n = snprintf(buffer, sizeof (buffer),
                     "/proc/%s/cmdline", process->d_name);
        if (n <= 0)
            continue;

        if ((cmdline = open(buffer, O_RDONLY)) < 0)
            continue;
        if (read(cmdline, command, sizeof(command)) <= 0) {
            close(cmdline);
            continue;
        }

        if (!strcmp(command, executable))
            kill(pid, signal_number);

        close(cmdline);
    }

    closedir(processes);
    return;
}

#endif
