#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
    header->date             = NULL;
    header->server           = NULL;
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
    header->date             = NULL;
    header->server           = NULL;
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

    content->content_is_loaded = false;
    content->file_path         = NULL;
    content->file_fd           = NO_FD;
    content->file_offset       = 0;
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
// HTTP SPECIAL TYPES-RELATED FUNCTIONS
// -----------------------------------------------------------------------------

// Currently assigning the code value in the enum
int getHttpCodeValue (const HttpCode http_code)
{
    return (int) http_code;
}

char* getHttpCodeDescription (const HttpCode http_code)
{
    switch (http_code)
    {
        case HTTP_200:
            return "OK";
        case HTTP_400:
            return "Bad request";
        case HTTP_401:
            return "Unauthorized";
        case HTTP_404:
            return "Not found";
        case HTTP_405:
            return "Method not allowed";
        case HTTP_411:
            return "Length required";
        case HTTP_414:
            return "Too-long URI";
        case HTTP_500:
            return "Internal server error";
        case HTTP_501:
            return "Not implemented";
        case HTTP_503:
            return "Service unavailable";
        case HTTP_505:
            return "HTTP version not supported";

        default:
        case HTTP_NO_CODE:
            return "No HTTP code";
    }
}

char* getHttpMethodAsString (const HttpMethod http_method)
{
    switch (http_method)
    {
        case HTTP_GET:
            return "HTTP_GET";
        case HTTP_HEAD:
            return "HTTP_HEAD";
        case HTTP_POST:
            return "HTTP_POST";
        case HTTP_PUT:
            return "HTTP_PUT";
        case HTTP_DELETE:
            return "HTTP_DELETE";
        case HTTP_CONNECT:
            return "HTTP_CONNECT";
        case HTTP_OPTIONS:
            return "HTTP_OPTIONS";
        case HTTP_TRACE:
            return "HTTP_TRACE";

        case HTTP_UNKNOWN_METHOD:
            return "HTTP_UNKNOWN_METHOD";

        default:
        case HTTP_NO_METHOD:
            return "No HTTP method";
    }
}

// -----------------------------------------------------------------------------
// HTTP OPTION FIELDS-RELATED FUNCTIONS
// -----------------------------------------------------------------------------

// TODO: improve this
char* getHttpServerDate ()
{
    static char date_string[256];

    // Get current GMT/UTC time and date
    time_t     current_time = time(NULL);
    struct tm* current_date = gmtime(&current_time);

    // Print them according to a specified format
    strftime(date_string, 256, HTTP_TIME_FORMAT_STR, current_date);

    return date_string;
}

// -----------------------------------------------------------------------------
// HTTP ANSWER MESSAGE PREPARATION
// -----------------------------------------------------------------------------

// Set general fields, used by all kinds of HTTP answers 
void prepareGeneralHttpAnswer (HttpMessage* answer, HttpCode http_code)
{
    // Initialize the answer structure (its body is set as empty)
    initAnswerHttpMessage(answer, HTTP_V1_1, http_code);

    // Set some general header fields
    answer->header->server = "MyAwesomeWebServer"; // TODO: set server name
    answer->header->date   = getHttpServerDate();
}

// Set fields required for a (generic) HTTP error answer
void prepareHttpError (HttpMessage* answer, HttpCode http_code)
{
    prepareGeneralHttpAnswer(answer, http_code);

    // Set header fields
    answer->header->content_length = 0;
}

// Set fields required for a (generic) HTTP valid answer
void prepareHttpValidAnswer (HttpMessage* request, HttpMessage* answer, File* file)
{
    prepareGeneralHttpAnswer(answer, HTTP_200);

    // Set header fields
    answer->header->content_length   = file->size;
    answer->header->content_type     = file->type;
    answer->header->content_encoding = file->encoding == ENCODING_GZIP
                                     ? "gzip"
                                     : "identity";

    // Set body fields (HEAD requests expect no body)
    // Curently, only GET and HEAD are supported
    if (request->header->method == HTTP_GET)
    {
        // If file content is not loaded, the server must know it, and use the path
        if (file->state == STATE_NOT_LOADED)
        {
            answer->content->length            = file->size;
            answer->content->file_path         = file->path;
            answer->content->content_is_loaded = false;
        }
        else
        {
            answer->content->length            = file->size;
            answer->content->body              = file->content;
            answer->content->content_is_loaded = true;
        }
    }
}

// -----------------------------------------------------------------------------
// HTTP REQUEST PARSING AND ANSWERING
// -----------------------------------------------------------------------------

// Parse a HTTP request from a buffer, and set the various fields of the given HttpMessage
// Return the HTTP code of the answer to produce
HttpCode parseHttpRequest (HttpMessage* request, char* buffer)
{
    // Clear the request message structure
    initRequestHttpMessage(request);

    int http_code = fillHttpHeaderWith(request->header, buffer);
    // TODO: handle body data

    return http_code;
}

// Produce a HTTP answer from a parsed request
void produceHttpAnswerFromRequest (HttpMessage* answer, HttpMessage* request, FileCache* cache)
{
    // TODO: use a better semantic for request parsing errors

    // If there has been an error while parsing the request, produce an error message
    if (request->header->code != HTTP_200)
    {
        prepareHttpError(answer, request->header->code);
        return;
    }

    // If the HTTP version is 1.0 or 2.0, it is not supported by the server
    // Thus, in such a case, answer with an error 505
    if (request->header->version != HTTP_V1_1)
    {
        prepareHttpError(answer, HTTP_505);
        return;
    }

    // If the method is unknown, answer with an error 400
    if (request->header->method == HTTP_UNKNOWN_METHOD)
    {
        prepareHttpError(answer, HTTP_400);
        return;
    }

    // If the method is neither GET nor HEAD, it is not implemented (yet)
    // Thus, in such a case, answer with an error 501
    if (request->header->method != HTTP_GET
    &&  request->header->method != HTTP_HEAD)
    {
        prepareHttpError(answer, HTTP_501);
        return;
    }

    // If the specified path is not the standard one, answer with an error 400
    if (request->header->requestType != HTTP_ORIGIN_FORM)
    {
        prepareHttpError(answer, HTTP_400);
        return;
    }

    // Otherwise, try to fetch the requested file
    File* requested_file = findFileInCache(cache, request->header->requestTarget);

    // If the file is not found, answer with an error 404
    if (requested_file == NOT_FOUND)
    {
        prepareHttpError(answer, HTTP_404);
        return;
    }

    // If it is found, correctly answer with a 200 code
    prepareHttpValidAnswer(request, answer, requested_file);
}

// -----------------------------------------------------------------------------
// HTTP ANSWER HEADER FILLING
// -----------------------------------------------------------------------------

// Return the number of bytes actually written in the given buffer
int writeHttpAnswerFirstLine (const HttpHeader* answer_header,
                              char* answer_header_buffer, const int buffer_max_length)
{
    // Expected format: <version> <code> <message> (CLRF)
    return snprintf(answer_header_buffer, buffer_max_length, "%s %d %s\r\n",
                    HTTP_SERVER_VERSION,
                    getHttpCodeValue(answer_header->code),
                    getHttpCodeDescription(answer_header->code)); 
}

// Return the number of bytes actually written in the given buffer
int writeHttpOptionFields (const HttpHeader* answer_header,
                           char* answer_header_buffer, const int buffer_max_length)
{
    int nb_bytes_written = 0;

    // Any answer field whose value is not NULL/>=0 is written in the buffer, each ending by (CLRF)
    if (answer_header->content_length >= 0)
        nb_bytes_written += snprintf(answer_header_buffer,
                                     buffer_max_length - nb_bytes_written,
                                     "Content-Length: %d\r\n", answer_header->content_length);

    if (answer_header->content_type != NULL)
        nb_bytes_written += snprintf(answer_header_buffer + nb_bytes_written,
                                     buffer_max_length - nb_bytes_written,
                                     "Content-Type: %s\r\n", answer_header->content_type);

    if (answer_header->content_encoding != NULL)
        nb_bytes_written += snprintf(answer_header_buffer + nb_bytes_written,
                                     buffer_max_length - nb_bytes_written,
                                     "Content-Encoding: %s\r\n", answer_header->content_encoding);

    if (answer_header->date != NULL)
        nb_bytes_written += snprintf(answer_header_buffer + nb_bytes_written,
                                     buffer_max_length - nb_bytes_written,
                                     "Date: %s\r\n", answer_header->date);

    if (answer_header->server != NULL)
        nb_bytes_written += snprintf(answer_header_buffer + nb_bytes_written,
                                     buffer_max_length - nb_bytes_written,
                                     "Server: %s\r\n", answer_header->server);

    return nb_bytes_written;   
}

// Return the number of bytes actually written in the given buffer
int fillHttpAnswerHeaderBuffer (HttpMessage* answer,
                                char* answer_header_buffer, const int buffer_max_length)
{
    // Fill the buffer with the (special) first line + all the filled option fields
    int nb_bytes_written = 0;

    nb_bytes_written += writeHttpAnswerFirstLine(answer->header, answer_header_buffer, buffer_max_length);
    nb_bytes_written += writeHttpOptionFields(answer->header, answer_header_buffer + nb_bytes_written,
                                              buffer_max_length - nb_bytes_written);

    // Add a blank line separating the header from the body
    nb_bytes_written += snprintf(answer_header_buffer + nb_bytes_written,
                                 buffer_max_length - nb_bytes_written,
                                 "\r\n");

    return nb_bytes_written;
}
