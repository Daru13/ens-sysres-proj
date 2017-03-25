#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "toolbox.h"
#include "server.h"

// -----------------------------------------------------------------------------
// BASIC WEB SOCKET FUNCTIONS
// -----------------------------------------------------------------------------

int createWebSocket ()
{
    int sockfd = socket(AF_INET,     /* IPv4 */
                        SOCK_STREAM, /* TCP  */
                        0);
    if (sockfd < 0)
        handleErrorAndExit("socket() failed");

    return sockfd;
}

/*
void connectWebSocket (const int sockfd, const struct sockaddr *address)
{
    int return_value = connect(sockfd, address, sizeof address);
    if (return_value < 0)
    {
        perror("connect() failed");
        exit(1);
    }
}
*/

struct sockaddr_in getLocalAddress (const int port)
{
    struct sockaddr_in address;

    address.sin_family      = AF_INET;
    address.sin_port        = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;
    memset(address.sin_zero, 0, 8);

    return address;
}

void bindWebSocket (const int sockfd, const struct sockaddr *address)
{
    int return_value = bind(sockfd, address, sizeof address);
    if (return_value < 0)
        handleErrorAndExit("bind() failed");
}

void listenWebSocket (const int sockfd, const int queue_max_length)
{
    int return_value = listen(sockfd, queue_max_length);
    if (return_value < 0)
        handleErrorAndExit("listen() failed");
}

int acceptWebSocket (const int sockfd, struct sockaddr* address)
{
    socklen_t address_length = sizeof address;

    int clientfd = accept(sockfd, address, &address_length);
    if (clientfd < 0)
        handleErrorAndExit("accept() failed");

    return clientfd;
}

// -----------------------------------------------------------------------------
// SERVER-RELATED STRUCTURE(S) HANDLING
// -----------------------------------------------------------------------------

// Noet that this function does not initialize everything;
// It should always be followed by a call to initServer()!
Server* createServer ()
{
    Server* new_server = malloc(sizeof(Server));
    if (new_server == NULL)
        handleErrorAndExit("malloc() failed in createServer()");

    return new_server;
}

void deleteServer (Server* server)
{
    free(server);
}

void initServer (Server* server, const int sockfd, const struct sockaddr address,
                 const ServParameters parameters)
{
    server->sockfd     = sockfd;
    server->address    = address;
    server->parameters = parameters;
}

// Initialize a server with default values
void defaultInitServer (Server* server)
{
    int                sockfd  = createWebSocket();
    struct sockaddr_in address = getLocalAddress(SERV_DEFAULT_PORT);

    ServParameters parameters;
    parameters.queue_max_length = SERV_DEFAULT_QUEUE_MAX_LENGTH;


    // TODO: fix the cast issue...
    initServer(server, sockfd, (struct sockaddr) address, parameters);
}

// -----------------------------------------------------------------------------
// MAIN FUNCTIONS: INIT, WAITING, CONNECTING, LOOPING AND READING
// -----------------------------------------------------------------------------

// Return the client-communication socket descriptor, and fill the structure
// client_adress with the right values (as accept() do)
int waitForClient (Server* server, struct sockaddr* client_address)
{
    int clientfd = acceptWebSocket(server->sockfd, client_address);

    return clientfd;
}

