//
// Created by mac on 2020/6/15.
//

#include "string_tools.h"
#include <sstream>

constexpr char hexmap[] = "0123456789abcdef";

namespace string_tools{

	std::string bin2hex(const uint8_t * hash,size_t length){
		std::stringstream ss;
		for (int i = 0; i < length; ++i) {
			ss << hexmap[(hash[i] & 0xF0) >> 4];
			ss << hexmap[hash[i] & 0x0F];
		}
		return ss.str();
	}

	void hex2bin(std::string in_str, uint8_t * out) {
		const char* in=in_str.c_str();
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

}
