#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "string_utils.h"

int string_is_empty(const char *s)
{
	return s == NULL || *s == 0;
}

static char* rtrim(char *str)
{
    if (str == NULL || *str == '\0')
    {
        return str;
    }
 
    char *p = str + strlen(str) - 1;
    while (p >= str  && isspace(*p))
    {
        *p = '\0';
        --p;
    }
 
    return str;
}

static char* ltrim(char *str)
{
    if (str == NULL || *str == '\0')
    {
        return str;
    }
 
    int len = 0;
    char *p = str;
    while (*p != '\0' && isspace(*p))
    {
        ++p;
        ++len;
    }
 
    memmove(str, p, strlen(str) - len + 1);
 
    return str;
}

char* string_trim(char *str)
{
    str = rtrim(str);
    str = ltrim(str);
    
    return str;
}

void hex2bin(const char* in, uint8_t * out) {
	size_t len = strlen(in);
	static const unsigned char TBL[] = {
		0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  58,  59,  60,  61,
		62,  63,  64,  10,  11,  12,  13,  14,  15,  71,  72,  73,  74,  75,
		76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
		90,  91,  92,  93,  94,  95,  96,  10,  11,  12,  13,  14,  15
	};
	static const unsigned char *LOOKUP = TBL - 48;
	const char* end = in + len;
	while(in < end) *(out++) = LOOKUP[*(in++)] << 4 | LOOKUP[*(in++)];
}
