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
#include "http.h"
#include "file_cache.h"
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

void disconnectClient (Client* client)
{
    printf("Disconnecting client (fd: %d)", client->fd);

    int success = close(client->fd);
    if (success < 0)
        handleErrorAndExit("close() failed in disconnectClient()");
}

void deleteClient (Client* client)
{
    // First, disconnect the client
    disconnectClient(client);

    // Then, free allocated structures
    free(client->request_buffer);
    deleteHttpMessage(client->http_request);

    free(client->answer_header_buffer);
    deleteHttpMessage(client->http_answer);

    free(client);
}

void initClient (Client* client, const int fd, const struct sockaddr_in address,
                 const ServParameters* parameters)
{
    client->fd      = fd;
    client->address = address;

    client->previous = NULL;
    client->next     = NULL;

    client->request_buffer_length = 0;
    client->request_buffer_offset = 0;

    client->request_buffer = malloc(parameters->read_buffer_size * sizeof(char));
    if (client->request_buffer == NULL)
        handleErrorAndExit("malloc() failed in initClient()");

    client->http_request = createHttpMessage();
    initRequestHttpMessage(client->http_request);
    // TODO: is init required?

    client->answer_header_buffer_length = 0;
    client->answer_header_buffer_offset = 0;

    client->answer_header_buffer = malloc(parameters->write_buffer_size * sizeof(char));
    if (client->answer_header_buffer == NULL)
        handleErrorAndExit("malloc() failed in initClient()");

    client->http_answer = createHttpMessage();
    // TODO: initialize?
}

char* getClientStateAsString (const ClientState state)
{
    switch (state)
    {
        case STATE_WAITING_FOR_REQUEST:
            return "WAITING_FOR_REQUEST";
        case STATE_PROCESSING_REQUEST:
            return "PROCESSING_REQUEST";
/*        case STATE_PRODUCING_ANSWER:
            return "PRODUCING_ANSWER";*/
        case STATE_ANSWERING:
            return "ANSWERING";

        default:
            return "UNKNOWN";
    }
}

void printClient (const Client* client)
{
    printSubtitle("Client (fd: %d)", client->fd);

    // Basic information on client
    printf("| state        : %s\n", getClientStateAsString(client->state));
    printf("| request      : ofs = %d, length = %d\n",
        client->request_buffer_length, client->request_buffer_offset);
    printf("| answer header: ofs = %d, length = %d\n",
        client->answer_header_buffer_length, client->answer_header_buffer_offset);
    printf("| answer body  : ofs = %d, length = %d\n",
        client->http_answer->content->offset, client->http_answer->content->length);
    printf("| requestTarget: %s\n", client->http_request->header->requestTarget != NULL ?
                                    client->http_request->header->requestTarget : "");
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

void disconnectServer (Server* server)
{
    printf("Disconnecting server (fd: %d)\n", server->sockfd);

    int success = close(server->sockfd);
    if (success < 0)
        handleErrorAndExit("close() failed in disconnectServer()");

    server->is_started = false;
}

void deleteServer (Server* server)
{
    // Start by disconnecting the server (i.e. closing the listening socket)
    disconnectServer(server);

    // Delete all the clients
    Client* current_client = server->clients;
    while (current_client != NULL)
    {
        Client* next_client = current_client->next;
        deleteClient(current_client);
        current_client = next_client;
    }

    // Delete the parameters structure
    free(server->parameters);

    // Delete the file cache
    deleteFileCache(server->cache);

    // FInallu delete the main structure
    free(server);
}

void initServer (Server* server, const int sockfd, const struct sockaddr_in address,
                 ServParameters* parameters)
{
    server->sockfd     = sockfd;
    server->address    = address;
    server->parameters = parameters;

    // When initialized, the server is considered non-active
    server->is_started = false;

    // ...and it has no client yet
    server->clients = NULL;

    // ...nor it has any cached file
    server->cache = NULL;

    server->nb_clients = 0;
    server->max_fd     = sockfd;
}

// Initialize a server with default values
void defaultInitServer (Server* server)
{
    int                sockfd  = createWebSocket();
    struct sockaddr_in address = getLocalAddress(SERV_DEFAULT_PORT);

    ServParameters* parameters = malloc(sizeof(ServParameters));
    if (parameters == NULL)
        handleErrorAndExit("malloc() failed in defaultInitServer()");

    parameters->queue_max_length    = SERV_DEFAULT_QUEUE_MAX_LENGTH;
    parameters->max_nb_clients      = SERV_DEFAULT_MAX_NB_CLIENTS;
    parameters->read_buffer_size    = SERV_DEFAULT_READ_BUF_SIZE;
    parameters->write_buffer_size   = SERV_DEFAULT_WRITE_BUF_SIZE;
    parameters->root_data_directory = SERV_DEFAULT_ROOT_DATA_DIR;

    initServer(server, sockfd, address, parameters);
}

bool serverIsStarted (const Server* server)
{
    return server->is_started;
}

void printServer (const Server* server)
{
    printSubtitle("[Server]");
    printf("sockfd    : %d\n", server->sockfd);
    printf("is started: %s\n", server->is_started ? "true" : "false");
    printf("nb_clients: %d\n", server->nb_clients);
    printf("max_fd    : %d\n", server->max_fd);
    printf("\n");

    Client* current_client = server->clients;
    while (current_client != NULL)
    {
        printClient(current_client);
        current_client = current_client->next;
    }
       
    printf("\n");
}

// -----------------------------------------------------------------------------
// BASIC SERVER AND CLIENT-HANDLING FUNCTIONS
// -----------------------------------------------------------------------------

// Once a server is created and initialized, this must be called in order
// to make it active (i.e. listening for requests and waiting for clients)
void startServer (Server* server)
{
    if (serverIsStarted(server))
        handleErrorAndExit("startServer() failed: server is already started");

    // Load the files in the cache
    server->cache = buildCacheFromDisk(server->parameters->root_data_directory, 32000000);
    printFileCache(server->cache); // TODO: Debug/improve

    // Attach the local adress to the socket, and make it a listener
    bindWebSocket(server->sockfd, &server->address);
    listenWebSocket(server->sockfd, server->parameters->queue_max_length);

    // Once started, update the internal state of the server
    server->is_started = true;
}

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
    ServParameters* parameters = server->parameters;

    if (! serverIsStarted(server))
        handleErrorAndExit("acceptNewClient() failed: server is not started");

    if (server->nb_clients == parameters->max_nb_clients)
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

// -----------------------------------------------------------------------------
// READING FROM AND WRITING TO CLIENTS
// -----------------------------------------------------------------------------

void readFromClient (Server* server, Client* client)
{
    // Read data from the socket, and null-terminate the buffer
    printf("Reading up to %d bytes from client %d...\n",
           server->parameters->read_buffer_size - 1, client->fd);
    int nb_bytes_read = read(client->fd, client->request_buffer,
                             server->parameters->read_buffer_size - 1);
    client->request_buffer[nb_bytes_read] = '\0';

    // Debug printing
    printf("***** Buffer content below (%d bytes) *****\n", nb_bytes_read);
    printf("%s\n", client->request_buffer);

    // If the read() call returned 0 (no byte has been read), it means the
    // client has ended the connection, and can be removed from the list of clients
    if (nb_bytes_read == 0)
    {
        removeClientFromServer(server, client);
        return;
    }

    // Otherwise, analyze the data which has been read from the client
    processClientRequest(server, client);
}

// TODO: CLEAN THIS!
// TODO: do this in another thread?
void processClientRequest (Server* server, Client* client)
{
    client->state = STATE_PROCESSING_REQUEST;

    // TODO: start by checking whether the request is complete or not?

    // Step 1: analyze the request
    initRequestHttpMessage(client->http_request);
    parseHttpRequest(client->http_request, client->request_buffer);

    // Step 2: produce the answer
    // TODO: what about the default HTTP code?
    initAnswerHttpMessage(client->http_answer, HTTP_V1_1, HTTP_400);
    produceHttpAnswer(client->http_request, client->http_answer, server->cache,
                     client->answer_header_buffer, &(client->answer_header_buffer_length));
    client->answer_header_buffer_offset = 0;

    client->state = STATE_ANSWERING;
}

// Only write the HTTP header buffer on the socket
// Returns true if the buffer has been entirely written, false otherwise
bool writeHttpHeaderToClient (Server* server, Client* client)
{
    // Write the header data on the socket
    printf("(HEAD) Writing up to %lu bytes to client %d...\n",
           strlen(client->answer_header_buffer), client->fd);

    int remaining_bytes_to_write = client->answer_header_buffer_length
                                 - client->answer_header_buffer_offset;
    int nb_bytes_write = write(client->fd, client->answer_header_buffer,
                               remaining_bytes_to_write);

    // Debug printing
    printSubtitle("***** (HEAD) Buffer content below (%d bytes) *****\n", nb_bytes_write);
    printf("%.*s\n", nb_bytes_write, client->answer_header_buffer);

    // Update the header buffer offset
    client->answer_header_buffer_offset += nb_bytes_write;

    return client->answer_header_buffer_offset
        == client->answer_header_buffer_length;
}

// Only write the HTTP header buffer on the socket
// Returns true if the buffer has been entirely written, false otherwise
bool writeHttpContentToClient (Server* server, Client* client)
{
    HttpContent* answer_content = client->http_answer->content;

    // There is no body to write if its content is NULL
    // In such a case, we assume the body has been written (by returning true)
    if (answer_content->body == NULL)
    {
        printf("\nNO BODY WAS FOUND: NO CONTENT TO SEND...\n\n");
        return true;
    }
    
    // Write the body data on the socket
    int remaining_bytes_to_write = answer_content->length - answer_content->offset;
    printf("(BODY) Writing up to %d bytes to client %d...\n",
           remaining_bytes_to_write, client->fd);

    int nb_bytes_write = write(client->fd, answer_content->body,
                               remaining_bytes_to_write);

    // Debug printing
    printSubtitle("***** (BODY) Buffer content below (%d bytes) *****\n", nb_bytes_write);
    printf("%.*s\n", nb_bytes_write, answer_content->body);

    // Update the message body offset
    answer_content->offset += nb_bytes_write;

    return answer_content->offset == answer_content->length;
}

void writeToClient (Server* server, Client* client)
{
    // In a first time, send the HTTP header data
    bool header_is_sent = writeHttpHeaderToClient(server, client);

    // In a second time, if the header has been sent, send the HTTP body data
    bool body_is_sent = false;
    if (header_is_sent)
    {
        printf("\nNOW SENDING BODY DATA :)!\n\n");
        body_is_sent = writeHttpContentToClient(server, client);    
    }

    // If the whole HTTP answer has been sent (header + body),
    // the server is done answering the client, and waits for new requets from it
    if (header_is_sent && body_is_sent)
        client->state = STATE_WAITING_FOR_REQUEST;
}

// -----------------------------------------------------------------------------
// MAIN SERVER LOOP : WAITING FOR CLIENTS AND REQUESTS, AND ANSWERING THEM
// -----------------------------------------------------------------------------

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
