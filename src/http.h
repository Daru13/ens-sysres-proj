#ifndef __H_HTTP__
#define __H_HTTP__

#include "file_cache.h"

// Useful, finite types related to HTTP
typedef enum HttpVersion {
    HTTP_V1_0, // Not supported
    HTTP_V1_1,
    HTTP_V2_0, // Not supported
    HTTP_UNKNOWN_VERSION
} HttpVersion;

typedef enum HttpMethod {
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,

    // TODO: add more methods :)!

    HTTP_UNKNOWN_METHOD, // Unknown when parsing
    HTTP_NO_METHOD       // No method needed (i.e. outgoing message)
} HttpMethod;

typedef enum HttpCode {
    HTTP_200 = 200, // OK
    HTTP_400 = 400, // Bad request
    HTTP_401 = 401, // Unauthorized
    HTTP_404 = 404, // Not found
    HTTP_405 = 405, // Method not allowed
    HTTP_411 = 411, // Length required
    HTTP_414 = 414, // Too-long URI
    HTTP_500 = 500, // Internal server error
    HTTP_501 = 501, // Not implemented
    HTTP_503 = 503, // Service unavailable
    HTTP_505 = 505, // HTTP version not supported

    HTTP_NO_CODE // No code needed (i.e. incomming message)
} HttpCode;

typedef enum HttpRequestType {
    HTTP_ORIGIN_FORM,    // Standard
    HTTP_ABSOLUTE_FORM,  // For proxies, not implemented
    HTTP_AUTHORITY_FORM, // For CONNECT, not implemented
    HTTP_ASTERISK_FORM,  // Only for OPTIONS
    
    HTTP_NO_REQUEST_TYPE
} HttpRequestType;

// Structure representing a HTTP header
// It can be used for both incomming and outgoing messages!
typedef struct HttpHeader {
    HttpVersion version;
    HttpMethod  method;
    HttpCode    code; 

    HttpRequestType requestType; 
    char*           requestTarget;

    char* query;
    char* host;
    char* accept;
    int   content_length;
    char* content_type;
    char* content_encoding;
    char* date;
    char* server;
} HttpHeader;

// Structure representing a chunk of (text) data
typedef struct HttpContent {
    int   length;
    int   offset;
    char* body;
} HttpContent;

// Struture represeting a full HTTP message
typedef struct HttpMessage {
    HttpHeader*  header;
    HttpContent* content;
} HttpMessage;

// -----------------------------------------------------------------------------

#define HTTP_TIME_FORMAT_STR "%a, %d %b %Y %X GMT"
#define HTTP_SERVER_VERSION  "HTTP/1.1"

// -----------------------------------------------------------------------------

HttpHeader* createHttpHeader ();
void deleteHttpHeader (HttpHeader* header);
void initRequestHttpHeader (HttpHeader* header);
void initAnswerHttpHeader (HttpHeader* header,
                           const HttpVersion version, const HttpCode code);

HttpContent* createHttpContent ();
void deleteHttpContent (HttpContent* content);
void initEmptyHttpContent (HttpContent* content);

HttpMessage* createHttpMessage ();
void deleteHttpMessage (HttpMessage* message);
void initRequestHttpMessage (HttpMessage* message);
void initAnswerHttpMessage (HttpMessage* message,
                            const HttpVersion version, const HttpCode code);

void parseHttpRequest (HttpMessage* request, char* buffer);
void produceHttpAnswer (HttpMessage* request, HttpMessage* answer, const FileCache* cache,
                        char* answer_header_buffer, int* answer_header_buffer_length);

#endif
