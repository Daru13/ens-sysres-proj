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

void initClient (Client* client, const int fd, const struct sockaddr_in address,
                 const int read_buffer_size, const int write_buffer_size)
{
    client->fd      = fd;
    client->address = address;

    client->slot_index = NO_ASSIGNED_SLOT;

    client->read_buffer_message_length = 0;
    client->read_buffer_message_offset = 0;
    client->read_buffer                = malloc(read_buffer_size * sizeof(char));
    if (client->read_buffer == NULL)
        handleErrorAndExit("malloc() failed in initClient()");

    client->write_buffer_message_length = 0;
    client->write_buffer_message_offset = 0;
    client->write_buffer                = malloc(write_buffer_size * sizeof(char));
    if (client->write_buffer == NULL)
        handleErrorAndExit("malloc() failed in initClient()");
}

char* getClientStateAsText (const ClientState state)
{
    switch (state)
    {
        case STATE_WAITING_FOR_REQUEST:
            return "WAITING_FOR_REQUEST";

        case STATE_ANSWERING:
            return "ANSWERING";

        default:
            return "UNKNOWN";
    }
}

void printClient (const Client* client)
{
    // To complete?
    printSubtitle("Client (fd: %d)", client->fd);
    printf("| slot_index: %d\n", client->slot_index);
    printf("| state: %s\n", getClientStateAsText(client->state));
    printf("| read_buffer: ofs = %d, length = %d\n",
        client->read_buffer_message_length, client->read_buffer_message_offset);
    printf("| write_buffer: ofs = %d, length = %d\n",
        client->write_buffer_message_length, client->write_buffer_message_offset);
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
    printSubtitle("\n====== SERVER ======");
    printf("sockfd: %d\n", server->sockfd);
    printf("is started: %s\n", server->is_started ? "true" : "false");
    printf("nb_clients: %d\n", server->nb_clients);
    printf("max_fd: %d\n", server->max_fd);
    printf("\n");

    for (int i = 0; i < server->parameters.max_nb_clients; i++)
    {
        Client* current_client = server->clients[i];
        if (current_client == NULL)
            continue;

        printClient(current_client);
    }
        
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

// TODO: handle a proper client addition/removal, to avoid waisted space/long searches!

void addClientToServer (Server* server, Client* client)
{
    // Find the first free client slot, and add the new client therein
    int first_free_slot_index = 0;
    while (server->clients[first_free_slot_index] != NULL
       &&  first_free_slot_index < server->parameters.max_nb_clients)
        first_free_slot_index++;

    client->slot_index = first_free_slot_index;

    server->clients[first_free_slot_index] = client;
    (server->nb_clients)++;

    // Check if the new client has the (new) highest file descriptor
    int client_fd = client->fd;
    if (client_fd > server->max_fd)
        server->max_fd = client_fd;
}

void removeClientFromServer (Server* server, Client* client)
{
    printf("Client (fd: %d) is being removed.\n", client->fd);

    // Remove the client from the server's list of client
    // Also updates the higher file descriptor (since it can be the
    // file descriptor of the client which is being removed)
    server->clients[client->slot_index] = NULL;

    int max_fd = server->sockfd;
    for (int i = 0; i < server->nb_clients; i++)
    {
        Client* current_client = server->clients[i];
        if (current_client == NULL)
            continue;

        int current_fd = current_client->fd;
        if (current_fd > max_fd)
            max_fd = current_fd;
    }
    server->max_fd = max_fd;
    
    (server->nb_clients)--;
    
    // Actually delete the Client structure
    deleteClient(client);
}

// Return a new, initialized client structure by using accept()
// The Server structure is also modified accordingly!

// TODO: improve this!
// If the server has no more free client slot, fails, print an error, and return NULL
Client* acceptNewClient (Server* server)
{
    ServParameters parameters = server->parameters;

    if (! serverIsStarted(server))
        handleErrorAndExit("acceptNewClient() failed: server is not started");

    if (server->nb_clients == parameters.max_nb_clients)
    {
        printError("Warning: acceptNewClient() failed: no more free slot!\n");
        return NULL;
    }

    struct sockaddr_in address;
    int clientfd = acceptWebSocket(server->sockfd, &address);

    // Create and initialize a Client structure, and add it to the server
    Client* new_client = createClient();
    initClient(new_client, clientfd, address,
               parameters.read_buffer_size, parameters.write_buffer_size);
    addClientToServer(server, new_client);

    return new_client;
}

// TODO: Temporary; must be improved
void readFromClient (Server* server, Client* client)
{
    // Read data from the socket
    printf("Reading up to %d bytes from client %d...\n",
           server->parameters.read_buffer_size - 1, client->fd);
    int nb_bytes_read = read(client->fd, client->read_buffer,
                             server->parameters.read_buffer_size - 1);
    client->read_buffer[nb_bytes_read] = '\0';

    printf("***** Buffer content below (%d bytes) *****\n", nb_bytes_read);
    printf("%s\n", client->read_buffer);
/*
    for (int i = 0; i < nb_bytes_read; i++)
        printf("char %i is %d\n", i, client->read_buffer[i]);

    printf("strlen buffer: %lu\n", strlen(client->read_buffer));
*/
    // TODO: REMOVE THIS TEST :D!
    if (! strcmp(client->read_buffer, "write\r\n"))
        client->state = STATE_ANSWERING;

    // If the read() call returned 0 (no byte has been read), it means the
    // client has ended the connection.
    // Thus, it can be removed from the list of clients
    if (nb_bytes_read == 0)
        removeClientFromServer(server, client);
}

// TODO: Temporary; must be improved
void writeToClient (Server* server, Client* client)
{
    // TODO: REMOVE THIS TEST
    char* test_msg = "YOU LOST, CLIENT!\n";
    sprintf(client->write_buffer, test_msg, sizeof test_msg);
    client->write_buffer_message_length = sizeof test_msg;

    // Write the data on the socket
    printf("Writing up to %d bytes to client %d...\n",
           server->parameters.write_buffer_size - 1, client->fd);
    int nb_bytes_write = write(client->fd, client->write_buffer,
                               server->parameters.write_buffer_size - 1);
    client->write_buffer[nb_bytes_write] = '\0';

    printf("***** Buffer content below (%d bytes) *****\n", nb_bytes_write);
    printf("%s\n", client->write_buffer);

    // Update the client state
    client->write_buffer_message_offset += nb_bytes_write;
    if (client->write_buffer_message_offset >= client->write_buffer_message_length)
        client->state = STATE_WAITING_FOR_REQUEST;  
}

/* TODO: READ AND WRITE DELEGATE TO PROCESSES/THREADS/ETC?
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

// TODO: use poll() or epoll() at some point?
void handleClientRequests (Server* server)
{
    if (! serverIsStarted(server))
        handleErrorAndExit("handleClientRequests() failed: server is not started");

    // List of file descriptors to wait for them to be ready
    fd_set read_fds, write_fds;

    // Indefinitely loop, waiting for new clients OR requests
    for (;;)
    {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        // Always scan for the socket listening for new clients
        FD_SET(server->sockfd, &read_fds);

        // Also scan for all the possibly ready clients
        for (int i = 0; i < server->parameters.max_nb_clients; i++)
        {
            // TODO: use a better data structure to store clients...? Doubly-linked list?
            Client* current_client = server->clients[i];
            if (current_client == NULL)
                continue;

            if (current_client->state == STATE_WAITING_FOR_REQUEST)
                FD_SET(current_client->fd, &read_fds);
            if (current_client->state == STATE_ANSWERING)
                FD_SET(current_client->fd, &write_fds);
        }

        printf("Before select() [sockfd = %d, nb_clients = %d]:\n",
               server->sockfd, server->nb_clients);

        int nb_ready_fds = select(server->max_fd + 1, &read_fds, &write_fds, NULL, NULL);
        if (nb_ready_fds < 0)
            handleErrorAndExit("select() failed");

        // Some debug printing :)
        printServer(server);
        printf("%d file descriptor(s) ready!\n", nb_ready_fds);

        // Wait for a current client's input
        // TODO: handle write operations as well!  
        for (int i = 0; i < server->parameters.max_nb_clients; i++)
        {
            Client* current_client = server->clients[i];
            if (current_client == NULL)
                continue;

            // Read or write from/to ready clients
            int current_fd = current_client->fd;
            printf("Checking client %d...\n", current_fd);

            if (FD_ISSET(current_fd, &read_fds))
                readFromClient(server, current_client);

            if (FD_ISSET(current_fd, &write_fds))
                writeToClient(server, current_client);
        }

        // If sockfd is ready, accept a new client
        if (FD_ISSET(server->sockfd, &read_fds))
        {
            Client* new_client = acceptNewClient(server);
            printf("New client (fd = %d) has been accepted.\n", new_client->fd);
        }
    }  
}
