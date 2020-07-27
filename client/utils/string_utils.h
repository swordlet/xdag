#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H

int string_is_empty(const char *str);
char *string_trim(char *str);
void hex2bin(const char* in, uint8_t * out);

#endif // !_STRING_UTILS_H

