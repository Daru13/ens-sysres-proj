#ifndef __H_SERVER__
#define __H_SERVER__

#include <stdbool.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>

// Structures used tor epresent a server
typedef struct ServParameters {
	int queue_max_length;
	// ...
} ServParameters;


typedef struct Server {
	int                sockfd;
	struct sockaddr_in address;
	bool 		       is_started;

	ServParameters  parameters;
	

} Server;

// -----------------------------------------------------------------------------

// Defautl values concerning the server
#define SERV_DEFAULT_PORT 			  4242
#define SERV_DEFAULT_QUEUE_MAX_LENGTH 5

// Named, useful constants
#define FD_NO_CLIENT -1

// -----------------------------------------------------------------------------


int createWebSocket ();
// void connectWebSocket (const int sockfd, const struct sockaddr *address);
struct sockaddr_in getLocalAddress (const int port);
void bindWebSocket (const int sockfd, const struct sockaddr_in *address);
void listenWebSocket (const int sockfd, const int queue_max_length);
int acceptWebSocket (const int sockfd, struct sockaddr_in* address);

Server* createServer ();
void deleteServer (Server* server);
void initServer (Server* server, const int sockfd, const struct sockaddr_in address,
                 const ServParameters parameters);
void defaultInitServer (Server* server);

void startServer (Server* server);
int waitForClient (Server* server, struct sockaddr_in* client_address);
void handleClientRequests (Server* server);

#endif
