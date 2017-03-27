#include <stdio.h>
#include <stdlib.h>
#include "toolbox.h"
#include "server.h"
#include "main.h"

// -----------------------------------------------------------------------------

int main (const int argc, const char* argv[])
{
    // printTitle("Web server -- by Camille GOBERT, Hugo MANET");
    // printUsage(argv);

    Server* server = createServer();
    defaultInitServer(server);

    startServer(server);
    handleClientRequests(server);

    // deleteServer(server);

    return 0;
}
