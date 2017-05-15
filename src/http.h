#ifndef __H_HTTP__
#define __H_HTTP__

#include <sys/types.h>
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
    HTTP_POST,    // Not supported
    HTTP_PUT,     // Not supported
    HTTP_DELETE,  // Not supported
    HTTP_CONNECT, // Not supported
    HTTP_OPTIONS, // Not supported
    HTTP_TRACE,   // Not supported

    HTTP_UNKNOWN_METHOD, // Unknown when parsing
    HTTP_NO_METHOD       // No method needed (i.e. outgoing message)
} HttpMethod;

// Note: there are many more methods, but we are not supporting them yet
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

    HTTP_NO_CODE = -1 // No code needed (i.e. incomming message)
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

    // In case the content is not in memory
    bool  content_is_loaded;

    char* file_path;
    int   file_fd;
    off_t file_offset;
} HttpContent;

// Struture represeting a full HTTP message
typedef struct HttpMessage {
    HttpHeader*  header;
    HttpContent* content;
} HttpMessage;

// -----------------------------------------------------------------------------

#define HTTP_TIME_FORMAT_STR "%a, %d %b %Y %X GMT"
#define HTTP_SERVER_VERSION  "HTTP/1.1"

#define NO_FD -1

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

int getHttpCodeValue (const HttpCode http_code);
char* getHttpCodeDescription (const HttpCode http_code);
char* getHttpMethodAsString (const HttpMethod http_method);

char* getHttpServerDate ();

void prepareGeneralHttpAnswer (HttpMessage* answer, HttpCode http_code);
void prepareHttpError (HttpMessage* answer, HttpCode http_code);
void prepareHttpValidAnswer (HttpMessage* request, HttpMessage* answer, File* file);

HttpCode parseHttpRequest (HttpMessage* request, char* buffer);
void produceHttpAnswerFromRequest (HttpMessage* answer, HttpMessage* request, FileCache* cache);


int writeHttpAnswerFirstLine (const HttpHeader* answer_header,
                              char* answer_header_buffer, const int buffer_max_length);
int writeHttpOptionFields (const HttpHeader* answer_header,
                           char* answer_header_buffer, const int buffer_max_length);
int fillHttpAnswerHeaderBuffer (HttpMessage* answer,
                                char* answer_header_buffer, const int buffer_max_length);

#endif
