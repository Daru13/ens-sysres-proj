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
// HTTP REQUEST PARSING AND ANSWERING
// -----------------------------------------------------------------------------

// Parse a HTTP request from a buffer, and set the various fields of the given HttpMessage
void parseHttpRequest (HttpMessage* request, char* buffer)
{
    int http_code = fillHttpHeaderWith(request->header, buffer);
    // TODO: handle body data
}

// Produce an HTTP answer from a parsed request
void produceHttpAnswerFromRequest (HttpMessage* answer, HttpMessage* request)
{

}

// -----------------------------------------------------------------------------
// HTTP CODES-RELATED FUNCTIONS
// -----------------------------------------------------------------------------

// Currently assigning the code value in the enum
// A warning is displayed if the code is HTTP_NO_CODE (-1 returned)
int getHttpCodeValue (const HttpCode http_code)
{
    switch (http_code)
    {
        case HTTP_NO_CODE:
            printWarning("Warning: trying to retrieve the value of a HTTP_NO_CODE code!");
            return -1;

        // In any other case, simply return the value of the code
        default:
            return (int) http_code;
    }
}

// A warning is displayed if the code is HTTP_NO_CODE ("Unknwon HTTP code" returned)
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

        case HTTP_NO_CODE:
        default:
            printWarning("Warning: trying to retrieve the description of a HTTP_NO_CODE code!");
            return "Unknown HTTP code";
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
void prepareHttpValidAnswer (HttpMessage* answer, HttpCode http_code, File* file)
{
    prepareGeneralHttpAnswer(answer, http_code);

    // Set header fields
    answer->header->content_length   = file->size;
    answer->header->content_type     = file->type;
    answer->header->content_encoding = file->state == STATE_LOADED_COMPRESSED
                                     ? "gzip"
                                     : "identity";

    // Set body fields
    answer->content->length = file->size;
    answer->content->body   = file->content;
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

/*
// Return true if the file was found, false otherwise
bool fillHttpAnswerBody (HttpMessage* answer)
{
    // Fetch the requested file
    File* requested_file = findFileInCache(cache, path);
}
*/
