#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "toolbox.h"

// -----------------------------------------------------------------------------
// FORMATTED PRINTING
// -----------------------------------------------------------------------------

// Internal version only! 
// Allows a cascade of functions calls using a variable number of arguments :)
void _printColor (const char* color, const char* format, va_list args)
{
    printf("%s", color);
    vprintf(format, args);
    printf("%s", COLOR_RESET);
}

void printColor (const char* color, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    _printColor(color, format, args);

    va_end(args);
}

void printSubtitle (const char* format, ...)
{
    va_list args;
    va_start(args, format);

    _printColor(SUBTITLE_COLOR, format, args);
    printf("\n");

    va_end(args);
}

void printTitle (const char* format, ...)
{
    va_list args;
    va_start(args, format);

    static char underline[TITLE_UNDERLINE_LENGTH + 1];
    for (int i = 0; i < TITLE_UNDERLINE_LENGTH + 1; i++)
    {
        underline[i] = (i == TITLE_UNDERLINE_LENGTH)
                     ? '\0'
                     : '-';
    }

    printColor(TITLE_COLOR, format, args);
    printf("\n");
    printColor(TITLE_COLOR, underline);
    printf("\n");

    va_end(args);
}

void printError (const char* format, ...)
{
    va_list args;
    va_start(args, format);

    #ifdef COLOR_ON_STDERR
    fprintf(stderr, "%s", ERROR_COLOR);
    #endif

    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    #ifdef COLOR_ON_STDERR
    fprintf(stderr, "%s", COLOR_RESET);
    #endif

    va_end(args);
}
