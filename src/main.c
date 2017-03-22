#include <stdio.h>
#include <stdlib.h>
#include "toolbox.h"
#include "server.h"
#include "main.h"

void printUsage (const char* argv[])
{
    printColor(COLOR_BOLD_GREEN,
               "Usage: %s ...", argv[0]);
}

void printUsageAndExit (const char* argv[])
{
    printUsage(argv);
    exit(1);
}

// -----------------------------------------------------------------------------

int main (const int argc, const char* argv[])
{
    printTitle("Webserver");
    printSubtitle("Test: %d", 42);
    printError("This is an error!");
    printUsage(argv);

    return 0;
}
