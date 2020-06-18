//
// Created by mac on 2020/6/15.
//

#ifndef XDAG_STRING_TOOLS_H
#define XDAG_STRING_TOOLS_H

#include <string>

namespace string_tools
{
	std::string bin2hex(const uint8_t * hash,size_t length);
	void hex2bin(std::string in_str, uint8_t * out);
};


#endif //XDAG_STRING_TOOLS_H
