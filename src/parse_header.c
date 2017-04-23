#include <string.h>
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

typedef struct Option
{
    char* key;
    void* value;
}

// last Option is default case, it has NULL for key
void* doSwitch(const Option opt[], char* str, int compLen = -1)
{
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
{ NULL, HTTP_UNKNOWN_METHOD }
}

// I swear i'll put the buffer back in its state after i've dealt with it
int fillHeaderWith(HttpHeader* header, char* buffer)
{
    char* pos = buffer;
    
    char* end = strchr(pos, ' ');
    header->method = doSwitch(methodSwitch, pos, (end - pos + 1));
    
    pos = end + 1;
    end = strchr(pos, ' ');
    
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
}

