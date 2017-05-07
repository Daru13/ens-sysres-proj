#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <poll.h>
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
                 const ServParameters parameters)
{
    client->fd      = fd;
    client->address = address;

    // client->slot_index = NO_ASSIGNED_SLOT;
    client->previous = NULL;
    client->next     = NULL;

    client->read_buffer_message_length = 0;
    client->read_buffer_message_offset = 0;
    client->read_buffer                = malloc(parameters.read_buffer_size * sizeof(char));
    if (client->read_buffer == NULL)
        handleErrorAndExit("malloc() failed in initClient()");

    client->write_buffer_message_length = 0;
    client->write_buffer_message_offset = 0;
    client->write_buffer                = malloc(parameters.write_buffer_size * sizeof(char));
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
    //printf("| slot_index: %d\n", client->slot_index);
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

    // ...and it has no client yet
    server->clients = NULL;

    server->nb_clients = 0;
    server->max_fd     = sockfd;
}

// Initialize a server with default values
void defaultInitServer (Server* server)
{
    int                sockfd  = createWebSocket();
    struct sockaddr_in address = getLocalAddress(SERV_DEFAULT_PORT);

    ServParameters parameters;
    parameters.queue_max_length    = SERV_DEFAULT_QUEUE_MAX_LENGTH;
    parameters.max_nb_clients      = SERV_DEFAULT_MAX_NB_CLIENTS;
    parameters.read_buffer_size    = SERV_DEFAULT_READ_BUF_SIZE;
    parameters.write_buffer_size   = SERV_DEFAULT_WRITE_BUF_SIZE;
    parameters.root_data_directory = SERV_DEFAULT_ROOT_DATA_DIR;

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

    Client* current_client = server->clients;
    while (current_client != NULL)
    {
        printClient(current_client);
        current_client = current_client->next;
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
    // Case 1: it is the first client
    if (server->nb_clients == 0)
    {
        server->clients = client;

        client->previous = NULL;
        client->next     = NULL;
    }

    // Case 2: it is not the first client
    else
    {
        // The new client is inserted right before the pointed client
        client->previous = NULL;
        client->next     = server->clients;

        server->clients->previous = client;

        // It thus becomes the "first" pointed client
        server->clients = client;
    }

    // Check if the new client has the (new) highest file descriptor
    int client_fd = client->fd;
    if (client_fd > server->max_fd)
        server->max_fd = client_fd;

    // Increment the number of clients
    (server->nb_clients)++;
}

void removeClientFromServer (Server* server, Client* client)
{
    printf("Client (fd: %d) is being removed.\n", client->fd);

    // Remove the client from the doubly-linked list
    if (client->next     != NULL)
        client->next->previous = client->previous;
    if (client->previous != NULL)
        client->previous->next = client->next;

    // If the server pointed to this client (i.e. it was the first client),
    // make it point to the next one
    if (server->clients == client)
        server->clients = client->next;

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
    initClient(new_client, clientfd, address, parameters);
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
    printf("Writing up to %lu bytes to client %d...\n",
           strlen(client->write_buffer), client->fd);
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

void handleClientRequests (Server* server)
{
    if (! serverIsStarted(server))
        handleErrorAndExit("handleClientRequests() failed: server is not started");

    // Client reference, used when looping over all the clients
    Client*     current_client;
    //int         current_fd;
    ClientState current_state;

    // Indefinitely loop, waiting for new/ready clients
    for (;;)
    {
        struct pollfd* polled_sockets = calloc(server->nb_clients + 1,
                                               sizeof(struct pollfd));
        if (polled_sockets == NULL)
            handleErrorAndExit("calloc() failed in handleClientRequests");

        // Always poll the socket listening for new clients IN FIRST POSITION
        int nb_polled_sockets    = 1;
        polled_sockets[0].fd     = server->sockfd;
        polled_sockets[0].events = POLLIN;

        current_client = server->clients;
        while (current_client != NULL)
        {
            current_state = current_client->state;
            switch (current_state)
            {
                case STATE_WAITING_FOR_REQUEST:
                printf("adding client fd %d for POLLIN\n", current_client->fd);
                    polled_sockets[nb_polled_sockets].fd     = current_client->fd;
                    polled_sockets[nb_polled_sockets].events = POLLIN;
                    break;

                case STATE_ANSWERING:
                 printf("adding client fd %d for POLLOUT\n", current_client->fd);
                    polled_sockets[nb_polled_sockets].fd     = current_client->fd;
                    polled_sockets[nb_polled_sockets].events = POLLOUT;
                    break;

                default:
                    // Otherwise, the client should not be polled
                    polled_sockets[nb_polled_sockets].fd = POLL_NO_POLLING;
                    break;
            }

            nb_polled_sockets++;
            current_client = current_client->next;
        }

        printf("Before poll() [sockfd = %d, nb_clients = %d]:\n",
               server->sockfd, server->nb_clients);

        int nb_ready_sockets = poll(polled_sockets, nb_polled_sockets, POLL_NO_TIMEOUT);
        if (nb_ready_sockets < 0)
            handleErrorAndExit("poll() failed");

        // Keep track of the position in the pollfd array
        // The first one must be checked in the end, as it listens for new clients
        int polled_sockets_index = 1; 

        int nb_handled_sockets   = 0;

        // Some debug printing :)
        printServer(server);

        // Read/write from/to ready clients, according to poll() revents fields
        current_client = server->clients;
        while (current_client != NULL)
        {
            // In case of current client's deletion, next one must be saved now,
            // so that the lopping proprely continue even if it is deleted
            Client* next_client = current_client->next;
            printf("*** Polling check ***\ncurrent_client: %p\nnext_client: %p\nhandled/ready: %d/%d\n\n",
                (void*) current_client, (void*) next_client, nb_handled_sockets, nb_ready_sockets);

            current_state = current_client->state;

            printf("revents (IN: %d, OUT: %d, HUP: %d): %d\n",
                (int) POLLIN, (int) POLLOUT, (int) POLLHUP, polled_sockets[polled_sockets_index].revents);

            // Check if the client closed the socket (meaning it should be removed)
            if (POLLHUP & polled_sockets[polled_sockets_index].revents)
            {
                removeClientFromServer(server, current_client);
            }

            // Otherwise, check for regular read/write events
            else
            {
                switch (current_state)
                {
                    case STATE_WAITING_FOR_REQUEST:
                        if (POLLIN & polled_sockets[polled_sockets_index].revents) {
                            readFromClient(server, current_client);
                        
                            printf("POLLIN fd: %d\n", polled_sockets[polled_sockets_index].fd);
                            nb_handled_sockets++;
                        }
                        break;

                    case STATE_ANSWERING:
                        if (POLLOUT & polled_sockets[polled_sockets_index].revents) {
                            writeToClient(server, current_client);
                        
                            printf("POLLOUT fd: %d\n", polled_sockets[polled_sockets_index].fd);
                            nb_handled_sockets++;
                        }
                        break;

                    default:
                        break;
                }
            }

            // If all ready clients have been handled, exit this loop
            if (nb_handled_sockets == nb_ready_sockets)
                break;

            polled_sockets_index++;
            current_client = next_client;
        }

        // If the server's sockfd is ready, accept a new client
        if (POLLIN & polled_sockets[0].revents)
        {
            Client* new_client = acceptNewClient(server);
            printf("New client (fd = %d) has been accepted.\n", new_client->fd);
        }

        free(polled_sockets);
    }
}
