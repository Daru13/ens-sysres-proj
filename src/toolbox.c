#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include "toolbox.h"

// -----------------------------------------------------------------------------
// ERROR HANDLING
// -----------------------------------------------------------------------------

// errno is saved at the beginning, and restored + returned in the end
int handleError (const char* message)
{
    int errno_copy = errno;

    printError("An error occured!");
    perror(message);

    errno = errno_copy;
    return errno_copy;
}

void handleErrorAndExit (const char* message)
{
    handleError(message);
    exit(EXIT_FAILURE);
}

// -----------------------------------------------------------------------------
// USAGE-RELATED FUNCTIONS
// -----------------------------------------------------------------------------

void printUsage (const char* argv[])
{
    printColor(COLOR_BOLD_GREEN,
               "Usage: %s ...", argv[0]);
}

void printUsageAndExit (const char* argv[])
{
    printUsage(argv);
    exit(EXIT_FAILURE);
}

// -----------------------------------------------------------------------------
// FORMATTED PRINTING
// -----------------------------------------------------------------------------

// Internal version only! 
// Allows a cascade of functions calls using a variable number of arguments :)
static void _printColor (const char* color, const char* format, va_list args)
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

void printWarning (const char* format, ...)
{
    va_list args;
    va_start(args, format);

    #ifdef COLOR_ON_STDERR
    fprintf(stderr, "%s", WARNING_COLOR);
    #endif

    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    #ifdef COLOR_ON_STDERR
    fprintf(stderr, "%s", COLOR_RESET);
    #endif

    va_end(args);
}

// -----------------------------------------------------------------------------
// STRING AND PATHS-RELATED FUNCTIONS
// -----------------------------------------------------------------------------

bool stringsAreEqual (const char* string_1, const char* string_2)
{
    return strcmp(string_1, string_2) == 0;
}

char* getFreshStringCopy (const char* source_string)
{
    char* string_copy = malloc((strlen(source_string) + 1) * sizeof(char));
    if (string_copy == NULL)
        handleErrorAndExit("malloc() failed in getFreshStringCopy()");
    
    return strcpy(string_copy, source_string);
}

char* extractLastNameOfPath (const char* path)
{  
    // Find the last '/' character position
    int last_slash_index = 0;
    int current_index    = 1;
    while (path[current_index] != '\0')
    {
        if (path[current_index] == '/')
            last_slash_index = current_index;

        current_index++;
    }

    // Copy the part of the string following the last slash into a fresh one
    char* directory_name = malloc((current_index - last_slash_index + 1) * sizeof(char));
    if (directory_name == NULL)
        handleErrorAndExit("malloc() failed in extractDirectoryNameFromPath()");

    directory_name = strcpy(directory_name, path + last_slash_index + 1);

    return directory_name;
}

// Note: this function does not return a new string, but modify the path argument instead
// Return true if the name fits, false otherwise (in which case, nothing happens)
bool appendNameToPath (char* path, const char* name, const int path_max_length)
{
    int name_length = strlen(name);
    int path_length = strlen(path);

    bool path_ends_with_slash = path[path_length - 1] == '/'; 
    
    int new_path_length = path_length + name_length + (path_ends_with_slash ? 0 : 1);
    if (new_path_length - 1 > path_max_length)
        return false;

    sprintf(path + path_length, "%s%s",
            path_ends_with_slash ? "" : "/",
            name);

    return true;
}
