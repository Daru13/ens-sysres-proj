#ifndef __H_HTTP__
#define __H_HTTP__

// Useful, finite types related to HTTP
typedef enum HttpVersion {
    HTTP_V1_0, // Possibly (partially?) handled?
    HTTP_V1_1,
    HTTP_V2_0, // Not handled here!
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
    HTTP_100, // Continue
    HTTP_200, // OK
    HTTP_301, // Moved permanentlu
    HTTP_302, // Moved temporarly
    HTTP_400, // Bad request
    HTTP_401, // Unauthorized
    HTTP_403, // Forbidden
    HTTP_404, // Not found
    HTTP_405, // Method not allowed
    HTTP_500, // Internal server error
    HTTP_501, // Not implemented
    HTTP_505, // HTTP version not supported
    HTTP_509, // Bandwith limit exceeeded [often used for too many clients?]

    // TODO: Possibly add more methods?
    // Not sure we can handle many more in limited time, though

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
    char* requestTarget;
    char* query;
    char* host;
    char* accept;
    int   content_length;
    char* content_type;
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

#endif
