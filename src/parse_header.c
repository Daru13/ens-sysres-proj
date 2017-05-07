#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "toolbox.h"
#include "http.h"
#include "parse_header.h"

// -----------------------------------------------------------------------------
// INTERNAL MACROS AND TYPES
// -----------------------------------------------------------------------------

#define ISOLATE(ptr, code) {\
    char ISOLATE_save = *ptr;\
    *ptr = '\0';\
    code;\
    *ptr = ISOLATE_save;\
}

// #define ISOLATE_CALL(ptr, code, returnValue) ISOLATE(ptr, returnValue = code)

#define CHECK_SYNTAX {\
    if (end == NULL)\
        return 400;\
}

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
// Note, though, than differnet field may share the same set of accepted values
Option methodSwitch[] = {
    { "GET",  HTTP_GET },
    { "HEAD", HTTP_HEAD },
    { "POST", HTTP_POST },
    { NULL,   HTTP_UNKNOWN_METHOD }
};

Option versionSwitch[] = {
    { "HTTP/1.0", HTTP_V1_0 },
    { "HTTP/1.1", HTTP_V1_1 },
    { "HTTP/2.0", HTTP_V2_0 },
    { NULL,       HTTP_UNKNOWN_VERSION }
};

Option headerSwitch[] = {
    { "HOST",   HEAD_HOST },
    { "ACCEPT", HEAD_ACCEPT },
    { NULL,     HEAD_UNKNOWN }
};

// -----------------------------------------------------------------------------
// HEADER FILLING FROM REQUEST BUFFER
// -----------------------------------------------------------------------------

/**
 * In @opt, last Option is default, it has NULL for key.
 * If case_unsensitive is set to true, the @key entries of @opt should be UPPERCASE or won't be recognized.
 */
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
        upperCase = malloc((compLen+1)*sizeof(char));
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
        if (strcmp(opt[optActu].key, str) == 0)
            return opt[optActu].value;
    return opt[optActu].value;
}

// I swear i'll put the buffer back in its state after i've dealt with it
int fillHeaderWith (HttpHeader* header, char* buffer)
{
    char* pos = buffer;
    
    char* end = strchr(pos, ' ');
    CHECK_SYNTAX
    header->method = getOptionValueFromString(methodSwitch, pos, true, end);
    
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
            header->requestTarget = malloc(len*sizeof(char));
            strncpy(header->requestTarget, pos, len-1);
            header->requestTarget[len-1] = '\0';
        }
        else
        {
            int len = queryPos-pos+1;
            header->requestTarget = malloc(len*sizeof(char));
            strncpy(header->requestTarget, pos, len-1);
            header->requestTarget[len-1] = '\0';
            header->query = malloc((end-queryPos)*sizeof(char));
            strncpy(header->query, queryPos+1, (end-queryPos-1));
            header->query[end-queryPos-1] = '\0';
        }
    }
    
    pos = end + 1;
    end = strchr(pos, '\n');
    CHECK_SYNTAX
    
    header->version = getOptionValueFromString(versionSwitch, pos, true, end);
    
    if (header->version != HTTP_V1_1)
    {
        switch (header->version)
        {
            case HTTP_V1_0:
            case HTTP_V2_0:
                return 505;
            default:
                // Probably syntax error
                // TODO : check if it's not just a version we don't implement and don't have heard from, which SHOULD be a 505 error.
                return 400;
        }
    }

    pos = end+1;

    // NOW we should be at the start of the header fields.
    while (*pos != '\n')
    {
        char* fieldName = pos;
        end = strchr(pos, ':');
        CHECK_SYNTAX
        char* endName = end; 
        char* fieldValue = endName+1;
        while (*fieldValue == ' ' || *fieldValue == '\t')
            fieldValue++;
        end = strchr(fieldValue, '\n');
        CHECK_SYNTAX
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
                    return 400;
                header->host = malloc((lenValue+1)*sizeof(char));
                strncpy(header->host, fieldValue, lenValue);
                header->host[lenValue] = '\0';
                break;
            case HEAD_ACCEPT:
                if (header->accept != NULL)
                {
                    int lenInit = strlen(header->accept);
                    char* new = realloc(header->accept, (lenInit+1+lenValue+1)*sizeof(char));
                    if (new == NULL)
                        return 500;
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

    return 200;
}
