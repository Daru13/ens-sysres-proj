#include <string.h>
#include <ctype.h>

#include "http.h"

#define ISOLATE(string, position, code) {\
    char ISOLATE_save = (string)[position];\
    string[position] = '\0';\
    code;\
    string[position] = ISOLATE_save;\
}
#define ISOLATE_CALL(string, position, code, returnValue) {\
    char ISOLATE_save = (string)[position];\
    string[position] = '\0';\
    returnValue = code;\
    string[position] = ISOLATE_save;\
}
#define CHECK_SYNTAX {\
    if (end == NULL)\
        return 400;\
}

typedef struct Option
{
    char* key;
    void* value;
}

/**
 * In @opt, last Option is default, it has NULL for key.
 * If case_unsensitive is set to true, the @key entries of @opt should be UPPERCASE or won't be recognized.
 */
void* doSwitch(const Option opt[], char* str, bool case_unsensitive = false, int compLen = -1)
{
    if (case_unsensitive)
    {
        char* upperCase;
        if (compLen != -1)
            compLen = strlen(str)+1;
        upperCase = malloc((compLen+1)*sizeof(char));
        for (int i = 0; i < compLen; ++i)
            upperCase[i] = toupper(str[i]);
        upperCase[compLen] = '\0';

        void* ret = doSwitch(opt, upperCase);
        free(upperCase);
        return ret;
    }
    if (compLen != -1)
    {
        void* ret;
        ISOLATE_CALL(str, compLen, doSwitch(opt, str), ret);
        return ret;
    }
    int optActu = 0;
    for (; opt[optActu].key != NULL; optActu++)
        if (strcmp(opt[optActu].key, str) == 0)
            return opt[optActu].value;
    return opt[optActu].value;
}

Option methodSwitch[] = {
{ "GET", HTTP_GET },
{ "HEAD", HTTP_HEAD },
{ "POST", HTTP_POST },
{ NULL, HTTP_UNKNOWN_METHOD }
}

Option versionSwitch[] = {
{ "HTTP/1.0", HTTP_V1_0 },
{ "HTTP/1.1", HTTP_V1_1 },
{ "HTTP/2.0", HTTP_V2_0 },
{ NULL, HTTP_UNKNOWN_VERSION }
}

// I swear i'll put the buffer back in its state after i've dealt with it
int fillHeaderWith(HttpHeader* header, char* buffer)
{
    char* pos = buffer;
    
    char* end = strchr(pos, ' ');
    CHECK_SYNTAX
    header->method = doSwitch(methodSwitch, pos, true, (end - pos + 1));
    
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
        ISOLATE_CALL(pos, (end-pos+1), strchr(pos, '?'), queryPos);
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
            header->query = malloc((end-queryPos)*sizeof(char))
            strncpy(header->query, queryPos+1, (end-queryPos-1));
            header->query[end-queryPos-1] = '\0';
        }
    }
    
    pos = end + 1;
    end = strchr(pos, '\n');
    CHECK_SYNTAX
    
    header->version = doSwitch(optionSwitch, pos, true, (end-pos+1));
    
    if (header->version != HTTP_V1_1)
    {
        // Probably syntax error
        // TODO : check if it's not just a version we don't implement and don't have heard from, which SHOULD be a 505 error.
        switch (header->version)
        {
            case HTTP_V1_0:
            case HTTP_V2_0:
                return 505;
            default:
                return 400;
        }
    }
}

