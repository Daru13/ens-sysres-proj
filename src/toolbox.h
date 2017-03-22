#ifndef __H_TOOLBOX__
#define __H_TOOLBOX__

#include <stdarg.h>
#include <stdbool.h>

// Definitions of format codes used for formatted printing
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"

#define COLOR_BOLD         "\x1b[1;37m"
#define COLOR_BOLD_RED     "\x1b[1;31m"
#define COLOR_BOLD_GREEN   "\x1b[1;32m"
#define COLOR_BOLD_YELLOW  "\x1b[1;33m"
#define COLOR_BOLD_BLUE    "\x1b[1;34m"
#define COLOR_BOLD_MAGENTA "\x1b[1;35m"
#define COLOR_BOLD_CYAN    "\x1b[1;36m"

#define COLOR_RESET   "\x1b[0m"

// Misc. parameters about formatted printing
#define TITLE_UNDERLINE_LENGTH 80
#define TITLE_COLOR            COLOR_BOLD_CYAN
#define SUBTITLE_COLOR         COLOR_BOLD

#define COLOR_ON_STDERR        /* comment to remove */
#define ERROR_COLOR            COLOR_BOLD_RED

// -----------------------------------------------------------------------------

void printColor (const char* color, const char* format, ...);
void printSubtitle (const char* format, ...);
void printTitle (const char* format, ...);
void printError (const char* format, ...);

#endif
