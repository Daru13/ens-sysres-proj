#include <stdio.h>
#include <stdlib.h>
#include "toolbox.h"
#include "parse_header.h"
#include "http.h"

// -----------------------------------------------------------------------------
// HTTP MESSAGES CREATION, DELETION, INITIALIZATION
// -----------------------------------------------------------------------------

HttpHeader* createHttpHeader ()
{
    HttpHeader* new_header = calloc(1, sizeof(HttpHeader));
    if (new_header == NULL)
        handleErrorAndExit("malloc() failed in createHttpHeader()");

    return new_header;
}

void deleteHttpHeader (HttpHeader* header)
{
    // TODO: free tons of strings (or other kinds of objects), used by header fields?
    free(header);
}

// Made for headers of incomming messages (i.e. progressively filled via parsing)
void initRequestHttpHeader (HttpHeader* header)
{
    header->version = HTTP_UNKNOWN_VERSION;
    header->method  = HTTP_UNKNOWN_METHOD;
    header->code    = HTTP_NO_CODE;
    
    header->requestType   = HTTP_NO_REQUEST_TYPE;
    
    header->requestTarget  = NULL;
    header->query          = NULL;
    header->host           = NULL;
    header->accept         = NULL;
    header->content_length = 0;
    header->content_type   = NULL;
}

// Made for headers of outgoing messages (i.e. built by the server to answer requests)
void initAnswerHttpHeader (HttpHeader* header,
                           const HttpVersion version, const HttpCode code)
{
    header->version = version;
    header->method  = HTTP_NO_METHOD;
    header->code    = code;

    header->requestType = HTTP_NO_REQUEST_TYPE;
    
    header->requestTarget  = NULL;
    header->query          = NULL;
    header->host           = NULL;
    header->accept         = NULL;
    header->content_length = 0;
    header->content_type   = NULL;
}

// -----------------------------------------------------------------------------

HttpContent* createHttpContent ()
{
    HttpContent* new_content = malloc(sizeof(HttpContent));
    if (new_content == NULL)
        handleErrorAndExit("malloc() failed in createHttpContent()");

    return new_content;
}

void deleteHttpContent (HttpContent* content)
{
    free(content);
}

void initEmptyHttpContent (HttpContent* content)
{
    content->length = 0;
    content->offset = 0;
    content->body   = NULL;
}

// -----------------------------------------------------------------------------

HttpMessage* createHttpMessage ()
{
    HttpMessage* new_message = malloc(sizeof(HttpMessage));
    if (new_message == NULL)
        handleErrorAndExit("malloc() failed in createHttpMessage()");

    // Also instanciate header and content structures
    new_message->header  = createHttpHeader();
    new_message->content = createHttpContent();

    return new_message;
}

void deleteHttpMessage (HttpMessage* message)
{
    deleteHttpHeader(message->header);
    deleteHttpContent(message->content);

    free(message);
}

// Made for incomming messages
void initRequestHttpMessage (HttpMessage* message)
{
    initRequestHttpHeader(message->header);
    initEmptyHttpContent(message->content);
}

// Made for outgoing messages
void initAnswerHttpMessage (HttpMessage* message,
                            const HttpVersion version, const HttpCode code)
{
    initAnswerHttpHeader(message->header, version, code);
    initEmptyHttpContent(message->content);
}

// -----------------------------------------------------------------------------
// HTTP REQUEST PARSING
// -----------------------------------------------------------------------------

// TODO: do this in another thread?
// Parse a HTTP request from a buffer, and set the various fields of the given HttpMessage
void parseHttpRequest (HttpMessage* request, char* buffer)
{
    int http_code /*?*/ = fillHttpHeaderWith(request->header, buffer);
    // TODO: what about the body?
}

// -----------------------------------------------------------------------------
// HTTP ANSWER PRODUCING
// -----------------------------------------------------------------------------

// TODO
