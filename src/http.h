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
}

// Structure representing a HTTP header
typedef struct HttpHeader {
    HttpVersion version;
    HttpMethod  method;
    HttpCode    code; 

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
    HttpHeader*  header;
    HttpContent* content;
} HttpMessage;

// -----------------------------------------------------------------------------



#endif
