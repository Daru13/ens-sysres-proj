// Macro definitions for using some recent signal handling functions
// #define _POSIX_C_SOURCE 200809L
#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "toolbox.h"
#include "server.h"
#include "main.h"

// -----------------------------------------------------------------------------

// The main server is in a global variables, so atexit's functions can access it
Server* _main_server = NULL;

// -----------------------------------------------------------------------------

void cleanClosing ()
{
    if (_main_server != NULL)
    {
        printf("Now cleaning and closing the server...\n");
        deleteServer(_main_server);
    }

    _main_server = NULL;
    printf("Cleaning done, goodbye!\n");
}

void handleSIGINT (int signal_id)
{
    // printf("Signal SIGINT received!\n");
    exit(EXIT_SUCCESS);
}

void installSIGINTHandler ()
{
    // Handle SIGINT, to exit the program when received
    struct sigaction sigint_handler;

    sigset_t signal_mask;
    sigfillset(&signal_mask);

    sigint_handler.sa_handler = handleSIGINT;
    sigint_handler.sa_flags   = 0;
    sigint_handler.sa_mask    = signal_mask;

    int success = sigaction(SIGINT, &sigint_handler, NULL);
    if (success < 0)
        handleErrorAndExit("sigaction() failed in installSIGINTHandler()");
}

int main (/*const int argc, const char* argv[]*/)
{
    // If there is a server, disconnect and close it at exit
    atexit(cleanClosing);

    // Handle SIGINT signal for clean server closing
    installSIGINTHandler();

    // Create, start and run the server
    _main_server = createServer();
    defaultInitServer(_main_server);
    startServer(_main_server);

    printServer(_main_server);

    // Start the main server loop
    handleClientRequests(_main_server);

    return 0;
}
