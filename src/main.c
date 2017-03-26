#include <stdio.h>
#include <stdlib.h>
#include "toolbox.h"
#include "server.h"
#include "main.h"

// -----------------------------------------------------------------------------

int main (const int argc, const char* argv[])
{
    printTitle("Webserver");
    printSubtitle("Test: %d", 42);
    printError("This is an error!");
    printUsage(argv);

    Server* server = createServer();
    defaultInitServer(server);
    startServer(server);
    handleClientRequests(server);

    return 0;
}
