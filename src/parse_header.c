#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "toolbox.h"
#include "http.h"
#include "file_cache.h"
#include "parse_header.h"

// -----------------------------------------------------------------------------
// INTERNAL MACROS AND TYPES
// -----------------------------------------------------------------------------

/*
#define ISOLATE(ptr, code) {\
    char ISOLATE_save = *ptr;\
    *ptr = '\0';\
    code;\
    *ptr = ISOLATE_save;\
}

// #define ISOLATE_CALL(ptr, code, returnValue) ISOLATE(ptr, returnValue = code)

#define CHECK_SYNTAX {\
    if (end == NULL)\
        return HTTP_400;\
}
*/

// -----------------------------------------------------------------------------

// Generic type for enum's values (basically integers)
typedef int OptionValue;

// An option associates a HTTP header field's string value (key) with its internally-used value
typedef struct Option
{
    char* key;
    OptionValue value;
} Option;

// List of handled header fields
typedef enum HttpHeaderField {
    HEAD_HOST,
    HEAD_ACCEPT,

    HEAD_UNKNOWN
} HttpHeaderField;

// Arrays of options
// Each different types of handled field should have its own array
// Note, though, than different field may share the same set of accepted values
Option accepted_methods[] = {
    { "GET",     HTTP_GET },
    { "HEAD",    HTTP_HEAD },
    { "POST",    HTTP_POST },
    { "PUT",     HTTP_PUT },
    { "DELETE",  HTTP_DELETE },
    { "CONNECT", HTTP_CONNECT },
    { "OPTIONS", HTTP_OPTIONS },
    { "TRACE",   HTTP_TRACE },
    { NULL,      HTTP_UNKNOWN_METHOD }
};

Option accepted_versions[] = {
    { "HTTP/1.0", HTTP_V1_0 },
    { "HTTP/1.1", HTTP_V1_1 },
    { "HTTP/2.0", HTTP_V2_0 },
    { NULL,       HTTP_UNKNOWN_VERSION }
};

Option accepted_option_fields[] = {
    { "HOST",   HEAD_HOST },
    { "ACCEPT", HEAD_ACCEPT },
    { NULL,     HEAD_UNKNOWN }
};

// -----------------------------------------------------------------------------
// REQUEST BUFFER CHECKING
// -----------------------------------------------------------------------------

// This value is used below, for faster checking
// It corresponds to the minimum-length of a buffer with just a first and a blank line
#define HTTP_HEADER_MIN_LENGTH 18 // bytes

// Return true if a full HTTP header is supposedly contained in the given buffer, false otherwise
// Note: only the double-CRLF is searched; no syntax is checked here!
bool bufferContainsFullHttpHeader (const char* buffer, const int start_offset, const int content_length)
{
    // If buffer contains less than 4 characters, immediately return false
    if (content_length < HTTP_HEADER_MIN_LENGTH)
        return false;

    // Search for an empty line (CRLF two times)
    int     index = content_length - 1;
    int end_index = start_offset - 3 >= 0
                  ? start_offset - 3
                  : 3;

    // The loop is done backwards, since there are high chances that
    // the pattern will be found at the end (e.g. if the header contains no body and is fully read)
    while (index >= end_index)
    {
        // Ignore chunks of 2 bytes if the first '\n' or '\n' are not found
        if (buffer[index - 3] == '\r'
        &&  buffer[index - 2] == '\n'
        &&  buffer[index - 1] == '\r'
        &&  buffer[index    ] == '\n')
        {
            return true;
        }

        index -= 2;
    }

    return false;
}  

// -----------------------------------------------------------------------------
// HEADER PARSING
// -----------------------------------------------------------------------------

// If bool to_uppercase is true, the string is converted to uppercase before trying to be matched
OptionValue findOptionValueFromString (const Option options[], char* string, bool to_uppercase)
{
    if (to_uppercase)
        string = convertStringToUppercase(string);

    int index = 0;
    while (options[index].key != NULL)
    {
        if (stringsAreEqual(options[index].key, string))
            return options[index].value;

        index++;
    }

    return options[index].value;
}

// TODO: very poor support of the HTTP 1.1 norm... But less bugged...
HttpCode parseHttpHeaderFirstLine (HttpHeader* header, char* buffer)
{
    int nb_bytes_parsed = 0;
    int nb_values_found = 0;

    char* http_method_string  = NULL;
    char* http_target_string  = NULL;
    char* http_version_string = NULL;

    // Find HTTP method
    nb_values_found = sscanf(buffer, "%ms", &http_method_string);
    if (nb_values_found != 1)
    {
        free(http_method_string);
        return HTTP_400; // Bad syntax
    }

    header->method = findOptionValueFromString(accepted_methods, http_method_string, false);
    if (header->method == HTTP_UNKNOWN_METHOD)
        return HTTP_501; // Not implemented

    int method_length = strlen(http_method_string);
    free(http_method_string);

    buffer = consumeLeadingStringWhiteSpace(buffer + method_length);

    // Find HTTP target
    nb_values_found = sscanf(buffer, "%ms", &http_target_string);
    if (nb_values_found != 1)
    {
        free(http_target_string);
        return HTTP_400; // Bad syntax
    }

    int path_length = strlen(http_target_string);
    if (path_length > MAX_PATH_LENGTH)
    {
        free(http_target_string);
        return HTTP_414; // Too-long URI
    }

    header->requestType   = HTTP_ORIGIN_FORM; // debug
    header->requestTarget = http_target_string;

    buffer = consumeLeadingStringWhiteSpace(buffer + path_length);
    
    // Find HTTP version
    nb_values_found = sscanf(buffer, "%ms", &http_version_string);
    if (nb_values_found != 1)
    {
        free(http_version_string);
        return HTTP_400; // Bad syntax
    }

    header->version = findOptionValueFromString(accepted_versions, http_version_string, false);
    if (header->version == HTTP_UNKNOWN_VERSION)
        return HTTP_505; // Version not supported

    int version_length = strlen(http_version_string);
    free(http_version_string);

    buffer = consumeLeadingStringWhiteSpace(buffer + version_length);

    if (buffer[0] != '\r' && buffer[1] != '\n')
        return HTTP_400;

    return HTTP_200;

    // TODO: parse option fields
    // TODO: Check for more issues
}

/**
 * In @opt, last Option is default, it has NULL for key.
 * If case_unsensitive is set to true, the @key entries of @opt should be UPPERCASE or won't be recognized.
 */

/* 
 * THERE ARE BUGS WITH THOSE FUNCTIONS...

OptionValue getOptionValueFromString (const Option opt[], char* str,
                                      bool case_unsensitive, char* posEnd)
{
    if (case_unsensitive)
    {
        char* upperCase;
        int compLen;
        if (posEnd == NULL)
            compLen = strlen(str)+1;
        else
            compLen = posEnd - str + 1;

        upperCase = malloc((compLen+1) * sizeof(char));
        if (upperCase == NULL)
                handleErrorAndExit("malloc() failed in getOptionValueFromString()");

        for (int i = 0; i < compLen; ++i)
            upperCase[i] = toupper(str[i]);
        upperCase[compLen] = '\0';

        OptionValue ret = getOptionValueFromString(opt, upperCase, false, NULL);
        free(upperCase);
        return ret;
    }

    if (posEnd != NULL)
    {
        OptionValue ret;
        ISOLATE(posEnd, ret = getOptionValueFromString(opt, str, false, NULL));
        return ret;
    }

    int optActu = 0;
    for (; opt[optActu].key != NULL; optActu++)
    {
        if (strcmp(opt[optActu].key, str) == 0)
            return opt[optActu].value;
    }
    return opt[optActu].value;
}

// I swear i'll put the buffer back in its state after i've dealt with it
HttpCode fillHttpHeaderWith (HttpHeader* header, char* buffer)
{
    char* pos = buffer;
    char* end = strchr(pos, ' ');
    CHECK_SYNTAX

    header->method = getOptionValueFromString(methodSwitch, pos, true, end - 1);
    
    pos = end + 1;
    end = strchr(pos, ' ');
    CHECK_SYNTAX
    
    if (end-pos == 1 && *pos == '*')
    {
        header->requestType = HTTP_ASTERISK_FORM;
        header->requestTarget = "*";
    }
    else
    {
        header->requestType = HTTP_ORIGIN_FORM;
        
        char* queryPos;
        ISOLATE(end, queryPos = strchr(pos, '?'));
        if (queryPos == NULL)
        {
            int len = end-pos+1;
            header->requestTarget = malloc(len * sizeof(char));
            if (header->requestTarget == NULL)
                handleErrorAndExit("malloc() failed in fillHttpHeaderWith()");

            strncpy(header->requestTarget, pos, len-1);
            header->requestTarget[len-1] = '\0';
        }
        else
        {
            int len = queryPos-pos+1;
            header->requestTarget = malloc(len * sizeof(char));
            if (header->requestTarget == NULL)
                handleErrorAndExit("malloc() failed in fillHttpHeaderWith()");

            strncpy(header->requestTarget, pos, len-1);
            header->requestTarget[len-1] = '\0';
            header->query = malloc((end-queryPos) * sizeof(char));
            if (header->query == NULL)
                handleErrorAndExit("malloc() failed in fillHttpHeaderWith()");

            strncpy(header->query, queryPos+1, (end-queryPos-1));
            header->query[end-queryPos-1] = '\0';
        }
    }
    
    pos = end + 1;
    end = strchr(pos, '\r') - 1;
    CHECK_SYNTAX
    
    header->version = getOptionValueFromString(versionSwitch, pos, true, end);
    if (header->version != HTTP_V1_1)
    {
        switch (header->version)
        {
            case HTTP_V1_0:
            case HTTP_V2_0:
                return HTTP_505;
            default:
                // Probably syntax error
                // TODO : check if it's not just a version we don't implement and don't have heard from, which SHOULD be a 505 error.
                return HTTP_400;
        }
    }

    pos = end + 2;

    // NOW we should be at the start of the header fields.
    while (*pos != '\n')
    {
        char* fieldName = pos;
        end = strchr(pos, ':');
        CHECK_SYNTAX
        printf("\n>>>>1\n");
        char* endName = end; 
        char* fieldValue = endName+1;
        while (*fieldValue == ' ' || *fieldValue == '\t')
            fieldValue++;
        end = strchr(fieldValue, '\n');
        CHECK_SYNTAX
        printf("\n>>>>2\n");
        char* endValue = end - 1; 
        while (*endValue == ' ' || *endValue == '\t')
            endValue--;
        endValue++;
        // value = [fieldValue : endValue[
        int lenValue = (endValue - fieldValue);
        
        HttpHeaderField headerField = getOptionValueFromString(headerSwitch, fieldName, true, endName+1);
        switch (headerField)
        {
            case HEAD_HOST:
            
                if (header->host)
                    free(header->host);

                header->host = malloc((lenValue+1) * sizeof(char));
                if (header->host == NULL)
                    handleErrorAndExit("malloc() failed in fillHttpHeaderWith()");

                strncpy(header->host, fieldValue, lenValue);
                header->host[lenValue] = '\0';
                break;

            case HEAD_ACCEPT:
                if (header->accept != NULL)
                {
                    int lenInit = strlen(header->accept);
                    char* new = realloc(header->accept, (lenInit+1+lenValue+1) * sizeof(char));
                    if (new == NULL)
                        handleErrorAndExit("realloc() failed in fillHttpHeaderWith()");

                    header->accept = new;
                    header->accept[lenInit] = ',';
                    header->accept[lenInit+1] = '\0';
                    strncat(header->accept, fieldValue, lenValue);
                    header->accept[lenInit+1+lenValue] = '\0';
                }
                break;

            default:
                break;
        }
    }

    return HTTP_200;
}
*/

