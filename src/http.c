#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toolbox.h"
#include "file_cache.h"
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
    // TODO: free tons of strings (or other kinds of objects), used by header fields
    free(header);
}

// Made for headers of incomming messages (i.e. progressively filled via parsing)
void initRequestHttpHeader (HttpHeader* header)
{
    header->version = HTTP_UNKNOWN_VERSION;
    header->method  = HTTP_UNKNOWN_METHOD;
    header->code    = HTTP_NO_CODE;
    
    header->requestType   = HTTP_NO_REQUEST_TYPE;
    
    header->query            = NULL;
    header->host             = NULL;
    header->accept           = NULL;
    header->requestTarget    = NULL;
    header->content_length   = 0;
    header->content_type     = NULL;
    header->content_encoding = NULL;
}

// Made for headers of outgoing messages (i.e. built by the server to answer requests)
void initAnswerHttpHeader (HttpHeader* header,
                           const HttpVersion version, const HttpCode code)
{
    header->version = version;
    header->method  = HTTP_NO_METHOD;
    header->code    = code;

    header->requestType = HTTP_NO_REQUEST_TYPE;
    
    header->query            = NULL;
    header->host             = NULL;
    header->accept           = NULL;
    header->requestTarget    = NULL;
    header->content_length   = 0;
    header->content_type     = NULL;
    header->content_encoding = NULL;
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

// Parse a HTTP request from a buffer, and set the various fields of the given HttpMessage
HttpCode parseHttpRequest (HttpMessage* request, char* buffer)
{
    return fillHttpHeaderWith(request->header, buffer);
    // TODO: what about the body?
}

// -----------------------------------------------------------------------------
// HTTP ANSWER PRODUCING
// -----------------------------------------------------------------------------

// TODO: this must be rewritten and improved!
void produceHttpAnswer (const HttpMessage* request, HttpMessage* answer, const FileCache* cache,
                        char* answer_header_buffer, int* answer_header_buffer_length)
{
    static char dummy[256];
    if (answer->header->code != HTTP_200)
    {
        answer->content->body = NULL;
        answer->content->length = 0;
        answer->content->offset = 0;

        answer->header->content_type   = "";
        answer->header->content_length = 0;

        sprintf(dummy, "HTTP/1.1 %d Error during analysis : unsupported request or syntax error\r\n\r\n", answer->header->code);
    }
    else
    {
        File* fetched_file = findFileInCache(cache, request->header->requestTarget);
        if (fetched_file != NOT_FOUND)
        {
            printf("\n\nFILE '%s' has been found!\n\n", request->header->requestTarget);
            //initAnswerHttpMessage(answer, HTTP_V1_1, HTTP_200);

            answer->content->body   = fetched_file->content;
            answer->content->length = fetched_file->size;
            answer->content->offset = 0;

            answer->header->content_type     = fetched_file->type;
            answer->header->content_length   = fetched_file->size;
            answer->header->content_encoding = fetched_file->state == STATE_LOADED_COMPRESSED ? "gzip" : "identity";

            sprintf(dummy, "HTTP/1.1 200\r\nContent-Length: %d\r\nContent-Type: %s\r\nContent-Encoding: %s\r\n\r\n",
                answer->header->content_length,
                answer->header->content_type,
                answer->header->content_encoding);
        }
        else
        {   
            printf("\n\nFILE '%s' NOT FOUND!\n\n", request->header->requestTarget);
            //initAnswerHttpMessage(answer, HTTP_V1_1, HTTP_404);

            answer->content->body   = NULL;
            answer->content->length = 0;
            answer->content->offset = 0;

            answer->header->content_type   = "";
            answer->header->content_length = 0;

            sprintf(dummy, "HTTP/1.1 404 File not found!\r\n\r\n");
        }
    }

    // TODO: ugly solution here, to be modified...
    answer_header_buffer        = strcpy(answer_header_buffer, dummy);
    *answer_header_buffer_length = strlen(dummy);
}
