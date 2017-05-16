#ifndef __H_PARSE_HEADER__
#define __H_PARSE_HEADER__

#include "http.h"

bool bufferContainsFullHttpHeader (const char* buffer, const int start_offset, const int content_length);

HttpCode parseHttpHeaderFirstLine (HttpHeader* header, char* buffer);
//HttpCode fillHttpHeaderWith (HttpHeader* header, char* buffer);

#endif
