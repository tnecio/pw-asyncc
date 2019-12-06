//
// Created by tom on 06.12.19.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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
