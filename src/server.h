#ifndef __H_SERVER__
#define __H_SERVER__

#include <netinet/in.h>

// Structures used tor epresent a server
typedef struct ServParameters {
	int queue_max_length;
	// ...
} ServParameters;


typedef struct Server {
	int sockfd;
	// ...
} Server;

// -----------------------------------------------------------------------------

int createWebSocket ();
void connectWebSocket (const int sockfd, const struct sockaddr *address);
struct sockaddr_in getLocalAddress (const int port);
void bindWebSocket (const int sockfd, const struct sockaddr *address);
void listenWebSocket (const int sockfd, const int queue_max_length);
int acceptWebSocket (const int sockfd, struct sockaddr* address);

#endif
