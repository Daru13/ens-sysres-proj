#ifndef __H_SERVER__
#define __H_SERVER__

#include <stdbool.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>

// Structure represeting a client (server-side)
typedef enum ClientState {
    STATE_WAITING_FOR_REQUEST,
    STATE_ANSWERING
} ClientState;

typedef struct Client {
    int                fd;
    struct sockaddr_in address;

    // TODO: change/remove?
    int slot_index; // Position in the server's client array
    
    ClientState state;
    char*       read_buffer;
    int         read_buffer_offset;
    char*       write_buffer;
    int         write_buffer_offset;
} Client;

// Structures used to represent a server
typedef struct ServParameters {
    int queue_max_length;
    int max_nb_clients;
    int read_buffer_size;
    int write_buffer_size;
    // ...
} ServParameters;

typedef struct Server {
    int                sockfd;
    struct sockaddr_in address;
    bool               is_started;

    Client**           clients;
    int                nb_clients;
    int                max_fd;

    ServParameters  parameters;
} Server;

// -----------------------------------------------------------------------------

// Default values concerning the server
#define SERV_DEFAULT_PORT             4242

#define SERV_DEFAULT_QUEUE_MAX_LENGTH 5
#define SERV_DEFAULT_MAX_NB_CLIENTS   64 
#define SERV_DEFAULT_READ_BUF_SIZE    512 // bytes
#define SERV_DEFAULT_WRITE_BUF_SIZE   512 // bytes

// Named, useful constants
#define FD_NO_CLIENT -1

#define NO_ASSIGNED_SLOT -1

// -----------------------------------------------------------------------------

int createWebSocket ();
// void connectWebSocket (const int sockfd, const struct sockaddr *address);
struct sockaddr_in getLocalAddress (const int port);
void bindWebSocket (const int sockfd, const struct sockaddr_in *address);
void listenWebSocket (const int sockfd, const int queue_max_length);
int acceptWebSocket (const int sockfd, struct sockaddr_in* address);

Client* createClient ();
void deleteClient (Client* client);
void initClient (Client* client, const int fd, const struct sockaddr_in address,
                 const int read_buffer_size, const int write_buffer_size);
Server* createServer ();
void deleteServer (Server* server);
void initServer (Server* server, const int sockfd, const struct sockaddr_in address,
                 const ServParameters parameters);
void defaultInitServer (Server* server);

void startServer (Server* server);
void addClientToServer (Server* server, Client* client);
void removeClientFromServer (Server* server, Client* client);
Client* acceptNewClient (Server* server);
void readFromClient (Server* server, Client* client);
void handleClientRequests (Server* server);

#endif
