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

    File*  f  = findFileInCache(cache, "a");
    File * f2 = findFileInCache(cache, "abcde/xyz");
    File * f3 = findFileInCache(cache, "subdir/fff");

    printf("f = %p, f2 = %p, f3 = %p\n", (void*) f, (void*) f2, (void*) f3);
    printFile(f,  0);
    printFile(f3, 0);
  
    // deleteFileCache(cache);

    return 0;
}
