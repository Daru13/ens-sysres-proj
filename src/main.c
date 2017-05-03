#include <stdio.h>
#include <stdlib.h>
#include "toolbox.h"
#include "file_cache.h"
#include "server.h"
#include "main.h"

// -----------------------------------------------------------------------------

int main (const int argc, const char* argv[])
{
    // printTitle("Web server -- by Camille GOBERT, Hugo MANET");
    // printUsage(argv);

    Server* server = createServer();
    defaultInitServer(server);
/*
    startServer(server);
    handleClientRequests(server);
*/
    // deleteServer(server);

    FileCache* cache = buildCacheFromDisk(server->parameters.root_data_directory,
                                          32 * 1000000);
    printFileCache(cache);
    //deleteFileCache(cache);

    return 0;
}
