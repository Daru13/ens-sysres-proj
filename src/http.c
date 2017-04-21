#include <stdio.h>
#include <stdlib.h>
#include "http.h"
#include "toolbox.h"

// -----------------------------------------------------------------------------
// HTTP MESSAGES CREATION, DELETION, INITIALIZATION
// -----------------------------------------------------------------------------

HttpHeader* createHttpHeader ()
{
    HttpHeader* new_header = calloc(sizeof(HttpHeader));
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
    
    header->requestType = HTTP_NO_REQUEST_TYPE;
/* Inutile : on utilise un calloc, tout est a zero de base.
    // TODO: init all fields to NULL (i.e. no value by default)
    header->field_example   = NULL;
    header->field_example_2 = NULL; // */
}

// Made for headers of outgoing messages (i.e. built by the server to answer requests)
void initAnswerHttpHeader (HttpHeader* header,
                           const HttpVersion version, const HttpCode code)
{
    header->version = version;
    header->method  = HTTP_NO_METHOD;
    header->code    = code;

    // TODO: init all fields to NULL (i.e. no value by default)
    header->field_example   = NULL;
    header->field_example_2 = NULL;
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
    // TODO: free the text buffer?
    free(content);
}

void initEmptyHttpContent (HttpContent* content)
{
    content->length = 0;
    content->text   = NULL;
}

// -----------------------------------------------------------------------------

HttpMessage* createHttpMessage ()
{
    HttpMessage* new_message = malloc(sizeof(HttpMessage));
    if (new_message == NULL)
        handleErrorAndExit("malloc() failed in createHttpMessage()");

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
