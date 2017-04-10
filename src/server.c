#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
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

void bindWebSocket (const int sockfd, const struct sockaddr_in *address)
{
/*
    printf("In bindWebScoket:\n");
    printf("sockfd: %d\nport: %d\naddr_ip: %d (%d)\naddr_family: %d (AF_INET = %d)\n",
            sockfd, ntohs(address->sin_port), address->sin_addr.s_addr, INADDR_ANY, address->sin_family, AF_INET);
*/
    int return_value = bind(sockfd, (const struct sockaddr*) address, sizeof(struct sockaddr_in));
    if (return_value < 0)
        handleErrorAndExit("bind() failed");
}

void listenWebSocket (const int sockfd, const int queue_max_length)
{
    int return_value = listen(sockfd, queue_max_length);
    if (return_value < 0)
        handleErrorAndExit("listen() failed");
}

int acceptWebSocket (const int sockfd, struct sockaddr_in* address)
{
    socklen_t address_length = sizeof address;

    int clientfd = accept(sockfd, (struct sockaddr*) address, &address_length);
    if (clientfd < 0)
        handleErrorAndExit("accept() failed");

    return clientfd;
}

// -----------------------------------------------------------------------------
// SERVER-RELATED STRUCTURE(S) HANDLING
// -----------------------------------------------------------------------------

Client* createClient ()
{
    Client* new_client = malloc(sizeof(Client));
    if (new_client == NULL)
        handleErrorAndExit("malloc() failed in createClient()");

    return new_client;
}

void deleteClient (Client* client)
{
    free(client);
}

void initClient (Client* client, const int fd, const struct sockaddr_in address)
{
    client->fd      = fd;
    client->address = address;
}

void printClient (Client* client)
{
    // To complete?
    printf("Client (fd: %d)\n", client->fd);
}

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

void initServer (Server* server, const int sockfd, const struct sockaddr_in address,
                 const ServParameters parameters)
{
    server->sockfd     = sockfd;
    server->address    = address;
    server->parameters = parameters;

    // When initialized, the server is considered non-active
    server->is_started = false;

    // It has no client at the beginning
    server->clients = malloc(parameters.max_nb_clients * sizeof(Client));
    if (server->clients == NULL)
        handleErrorAndExit("malloc() failed in initServer()");

    for (int i = 0; i < parameters.max_nb_clients; i++)
        server->clients[i] = NULL;

    server->nb_clients = 0;
    server->max_fd     = sockfd;
}

// Initialize a server with default values
void defaultInitServer (Server* server)
{
    int                sockfd  = createWebSocket();
    struct sockaddr_in address = getLocalAddress(SERV_DEFAULT_PORT);

    ServParameters parameters;
    parameters.queue_max_length  = SERV_DEFAULT_QUEUE_MAX_LENGTH;
    parameters.max_nb_clients    = SERV_DEFAULT_MAX_NB_CLIENTS;
    parameters.read_buffer_size  = SERV_DEFAULT_READ_BUF_SIZE;
    parameters.write_buffer_size = SERV_DEFAULT_WRITE_BUF_SIZE;

    initServer(server, sockfd, address, parameters);
}

bool serverIsStarted (const Server* server)
{
    return server->is_started;
}

void printServer (const Server* server)
{
    printSubtitle("\n----- SERVER -----");
    printf("sockfd: %d\n", server->sockfd);
    printf("is started: %s\n", server->is_started ? "true" : "false");
    printf("nb_clients: %d\n", server->nb_clients);
    printf("max_fd: %d\n", server->max_fd);
    printf("\n");

    for (int i = 0; i < server->nb_clients; i++)
        printClient(server->clients[i]);
    printf("\n");

    // Print parameters as well?
}

// -----------------------------------------------------------------------------
// MAIN FUNCTIONS: INIT, WAITING, CONNECTING, LOOPING AND READING
// -----------------------------------------------------------------------------

// Once a server is created and initialized, this must be called in order
// to make it active (i.e. listening for requests and waiting for clients)
void startServer (Server* server)
{
    if (serverIsStarted(server))
        handleErrorAndExit("startServer() failed: server is already started");

   // Attach the local adress to the socket, and make it a listener
    bindWebSocket(server->sockfd, &server->address);
    listenWebSocket(server->sockfd, server->parameters.queue_max_length);

    // Once started, update the internal state of the server
    server->is_started = true;
}

// Return a new, initialized client structure by using accept()
// The Server structure is also modified accordingly!

// TODO: improve this!
// If the server has no more free client slot, fails, print an error, and return NULL
Client* acceptNewClient (Server* server)
{
    if (! serverIsStarted(server))
        handleErrorAndExit("acceptNewClient() failed: server is not started");
    if (server->nb_clients == server->parameters.max_nb_clients)
    {
        printError("Warning: acceptNewClient() failed: no more free slot!\n");
        return NULL;
    }

    struct sockaddr_in address;
    int clientfd = acceptWebSocket(server->sockfd, &address);

    // Create and initialize a Client structure
    Client* new_client = createClient();
    initClient(new_client, clientfd, address);

    // Update the Server structure
    server->clients[server->nb_clients] = new_client;
    (server->nb_clients)++;

    // Check if it is the highest file descriptor
    if (clientfd > server->max_fd)
        server->max_fd = clientfd;

    return new_client;
}

// TODO: use poll() or epoll() at some point?
void handleClientRequests (Server* server)
{
    if (! serverIsStarted(server))
        handleErrorAndExit("handleClientRequests() failed: server is not started");

    // List of file descriptors to wait for them to be ready + timer for wait()
    fd_set read_fds, write_fds;
    // struct timeval timeout;

    char buffer[512 + 1]; // TODO: temporary, delegate read and write ops

    // Indefinitely loop, waiting for new clients OR requests
    for (;;)
    {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        // Always scan for the socket listening for new clients
        FD_SET(server->sockfd, &read_fds);

        // Also scan for all the possibly ready clients
        for (int i = 0; i < server->nb_clients; i++)
        {
            // In case of client removal + use of an array...
            // TODO: use a better data structure to store clients...? Doubly-linked list?
            // TODO: handle more than reading from clients!
            Client* current_client = server->clients[i];
            if (current_client != NULL)
                FD_SET(current_client->fd, &read_fds);
        }

        // Set up the timeout value (modified each time too); infinite time here
        // timeout.tv_sec  = -1;
        // timeout.tv_usec = -1;

        printf("Before select() [sockfd = %d, nb_clients = %d]:\n",
               server->sockfd, server->nb_clients);

        int nb_ready_fds = select(server->max_fd + 1, &read_fds, &write_fds, NULL, NULL);
        if (nb_ready_fds < 0)
            handleErrorAndExit("select() failed");

        // Some debug printing :)
        printServer(server);
        printf("%d file descriptor(s) ready!\n", nb_ready_fds);

        int nb_ready_clients_read = nb_ready_fds;

        /* TODO: READ AND WRITE DELEGATE TO PROCESSES/THREADS/ETC
         *
         * it may be an interesting idea to delegate read and write operations to
         * threads (for instance), so it doesn't block the current process to keep
         * listening to sockets/answering more requests in "parallel"?
         *
         * However, we are still limited by the network interface (i.e. if several
         * threads try to read or write from different sockets, will they likely be
         * paused and only processed one after another ; is there any advantage in
         * doing this...?)
         *
         * There may also be interesting flags to use, to reduce/avoid
         * such issues/latency!
         *
         * This should be discussed at some point, even though this is not important
         * at the current time :).
         */

        // Wait for a current client's input
        // TODO: handle write operations as well!  
        for (int i = 0; i < server->nb_clients; i++)
        {
            // It is useless to check for all clients if we already found all the ready ones
            if (nb_ready_clients_read == 0)
                break;

            Client* current_client = server->clients[i];
            if (current_client == NULL)
                continue;

            // read() call on each ready client's socket fd
            int current_fd = current_client->fd;
            printf("Checking client %d...\n", current_fd);

            if (FD_ISSET(current_fd, &read_fds))
            {
                nb_ready_clients_read--;
                
                // TODO: to delegate; this is just a temporary reading test!
                printf("Reading up to 512 bytes from client %d...\n", current_fd);
                
                int nb_bytes_read = read(current_fd, &buffer, 512);
                buffer[nb_bytes_read] = '\0';

                printf("***** Buffer content below (%d bytes) *****\n", nb_bytes_read);
                printf("%s\n", buffer);
            }
        }

        // If sockfd is ready, accept a new client
        if (FD_ISSET(server->sockfd, &read_fds))
        {
            nb_ready_clients_read--;

            Client* new_client = acceptNewClient(server);
            printf("New client (fd = %d) has been accepted.\n", new_client->fd);
        }
    }  
}
