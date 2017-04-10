#ifndef __H_HTTP__
#define __H_HTTP__

#include <stdbool.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>

// Useful, finite types related to HTTP
typedef enum HttpVersion {
	HTTP_V1_0,
	HTTP_V1_1,
	HTTP_V2_0,
	HTTP_UNKNOWN_VERSION
} HttpVersion;

typedef enum HttpMethod {
	HTTP_GET,
	HTTP_POST
	// TODO: to complete
} HttpMethod;

// Structure representing a HTTP header
typedef struct HttpHeader {
	HttpVersion version;
	HttpMethod  method;

	char* field_example;
	char* field_example_2;
} HttpHeader;

// Structure representing a chunk of (text) data
typedef struct HttpContent {
	int   length;
	char* text;
} HttpContent;

// Struture represeting a full HTTP message
typedef struct HttpMessage {
	HttpHeader  header;
	HttpContent content;
} HttpMessage;

// -----------------------------------------------------------------------------



#endif
