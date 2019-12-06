//
// Created by tom on 06.12.19.
//

#ifndef _ERR_H_
#define _ERR_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "err.h"

void failure(int error_number, const char *fmt, ...)
{
    fprintf(stderr, "ERROR: ");

    va_list fmt_args;
    va_start(fmt_args, fmt);
    vfprintf(stderr, fmt, fmt_args);
    va_end (fmt_args);

    fprintf(stderr," (%d; %s)\n", error_number, strerror(error_number));
    exit(1);
}

#endif //_ERR_H_
