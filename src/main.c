// Weird workaround for having all signal handling tools we need
//#define _POSIX_C_SOURCE 200809L
#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include "toolbox.h"
#include "server.h"
#include "main.h"

#include <signal.h>

// -----------------------------------------------------------------------------

Server* server = NULL;

// To improve!
void cleanClosing ()
{
    printf("Now cleaning and closing the server...\n");
    if (server != NULL)
        deleteServer(server);

    server = NULL;
    printf("Cleaning done, goodbye!\n");
}

void handleSIGINT (int signal_id)
{
    printf("Signal SIGINT received!\n");
    exit(EXIT_SUCCESS);
}

int main (const int argc, const char* argv[])
{
    // If there is a server, disconnect and close it at exit
    atexit(cleanClosing);

    // Handle SIGINT, to exit the program when received
    struct sigaction sigint_handler;

    sigset_t signal_mask;
    sigfillset(&signal_mask);

    sigint_handler.sa_handler = handleSIGINT;
    sigint_handler.sa_flags   = 0;
    sigint_handler.sa_mask    = signal_mask;

    int success = sigaction(SIGINT, &sigint_handler, NULL);
    if (success < 0)
        handleErrorAndExit("sigaction() failed in main()");

    // Create, start and run the server
    server = createServer();
    defaultInitServer(server);
    startServer(server);

    handleClientRequests(server);

    return 0;
}
