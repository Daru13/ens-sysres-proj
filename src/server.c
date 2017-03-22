#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "toolbox.h"
#include "server.h"

// -----------------------------------------------------------------------------
// SOCKET AND CONNECTION
// -----------------------------------------------------------------------------

int createWebSocket ()
{
    int sockfd = socket(AF_INET,     /* IPv4 */
                        SOCK_STREAM, /* TCP  */
                        0);
    if (sockfd < 0)
    {
        perror("socket() failed");
        exit(1);
    }

    return sockfd;
}

void connectWebSocket (const int sockfd, const struct sockaddr *address)
{
    int return_value = connect(sockfr, address, sizeof address);
    if (return_value < 0)
    {
        perror("connect() failed");
        exit(1);
    }
}
