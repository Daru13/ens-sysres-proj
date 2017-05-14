#ifndef __H_SERVER__
#define __H_SERVER__

#include <stdbool.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <poll.h>
#include "http.h"

// Structure represeting a client (server-side)
typedef enum ClientState {
    STATE_WAITING_FOR_REQUEST,
    STATE_PROCESSING_REQUEST,
    STATE_ANSWERING
} ClientState;

typedef struct Client {
    int                fd;
    struct sockaddr_in address;

    // Clients form a doubly-linked list
    struct Client* previous;
    struct Client* next;
    
    ClientState state;

    // Buffer to read data
    char* request_buffer;
    int   request_buffer_length;
    int   request_buffer_offset;

    // Related HTTP request
    HttpMessage* http_request;

    // Buffer to write header data to send
    char* answer_header_buffer;
    int   answer_header_buffer_length;
    int   answer_header_buffer_offset;

    // Related HTTP answer 
    // Note: it contains a pointer to the body data to send
    HttpMessage* http_answer;
} Client;

// Structures used to represent a server
typedef struct ServParameters {
    int   queue_max_length;
    int   max_nb_clients;
    int   request_buffer_size;
    int   answer_header_buffer_size;
    char* root_data_directory;
    // ...
} ServParameters;

typedef struct Server {
    int                sockfd;
    struct sockaddr_in address;
    bool               is_started;

    Client*            clients;
    int                nb_clients;
    int                max_fd;

    FileCache* cache;

    ServParameters* parameters;
} Server;

// -----------------------------------------------------------------------------

// Default values concerning the server
#define SERV_DEFAULT_PORT                4242

#define SERV_DEFAULT_QUEUE_MAX_LENGTH    5
#define SERV_DEFAULT_MAX_NB_CLIENTS      64 
#define SERV_DEFAULT_REQUEST_BUF_SIZE    16384 // bytes
#define SERV_DEFAULT_ANS_HEADER_BUF_SIZE 2048  // bytes

#define SERV_DEFAULT_ROOT_DATA_DIR    "./www"

// Named, useful constants
#define POLL_NO_TIMEOUT  -1
#define POLL_NO_POLLING  -1

// -----------------------------------------------------------------------------

int createWebSocket ();
struct sockaddr_in getLocalAddress (const int port);
void bindWebSocket (const int sockfd, const struct sockaddr_in *address);
void listenWebSocket (const int sockfd, const int queue_max_length);
int acceptWebSocket (const int sockfd, struct sockaddr_in* address);

Client* createClient ();
void disconnectClient (Client* client);
void deleteClient (Client* client);
void initClient (Client* client, const int fd, const struct sockaddr_in address,
                 const ServParameters* parameters);
char* getClientStateAsString (const ClientState state);
void printClient (const Client* client);

Server* createServer ();
void disconnectServer (Server* server);
void deleteServer (Server* server);
void initServer (Server* server, const int sockfd, const struct sockaddr_in address,
                 ServParameters* parameters);
void defaultInitServer (Server* server);
bool serverIsStarted (const Server* server);
void printServer (const Server* server);

void startServer (Server* server);
void addClientToServer (Server* server, Client* client);
void removeClientFromServer (Server* server, Client* client);
Client* acceptNewClient (Server* server);

void readFromClient (Server* server, Client* client);
void processClientRequest (Server* server, Client* client);
bool writeHttpHeaderToClient (Client* client);
bool writeHttpContentToClient (Client* client);
void writeToClient (Server* server, Client* client);

void handleClientRequests (Server* server);

#endif
