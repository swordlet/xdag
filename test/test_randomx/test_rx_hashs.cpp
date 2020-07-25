#include <iostream>
#include <sstream>
#include "../../client/rx_hashs.h"

constexpr char hexmap[] = "0123456789abcdef";
inline std::string hash2hex(uint8_t * hash)
{
	std::stringstream ss;
	for (int i = 0; i < 32; ++i) {
		ss << hexmap[(hash[i] & 0xF0) >> 4];
		ss << hexmap[hash[i] & 0x0F];
	}
	return ss.str();
}

void test1(){
	std::string seed1="hello rx 1";
	std::string seed2="hello rx 33333";

	std::string str1="hello world 1";

	uint8_t buf[32]={0};
	rx_slow_hashs(seed1.c_str(), str1.c_str(), strlen(str1.c_str()), buf);
	std::cout << time(nullptr) << " rx slow hash with seed1 " << hash2hex(buf)  << std::endl;

	rx_slow_hashs(seed2.c_str(), str1.c_str(), strlen(str1.c_str()), buf);
	std::cout << time(nullptr) << " rx slow hash with seed2 " << hash2hex(buf)  << std::endl;

	rx_slow_hashs(seed1.c_str(),str1.c_str(),strlen(str1.c_str()),buf);
	std::cout << time(nullptr) << " get rx slow hash with seed1 " << hash2hex(buf)  << std::endl;

	rx_slow_hashs(seed2.c_str(),str1.c_str(),strlen(str1.c_str()),buf);
	std::cout << time(nullptr) << " get rx slow hash with seed2 " << hash2hex(buf)  << std::endl;
}

void test2(){
	uint8_t buf[32]={0};
	std::string seed1="hello rx 1";
	int i=0;
	while(i < 1024){
		i ++;
		char tmp[128]={0};
		sprintf(tmp,"hello world %d",i);
		rx_slow_hashs(seed1.c_str(), tmp, strlen(tmp), buf);
		std::cout << time(nullptr) << " get rx slow hash with seed1 " << hash2hex(buf)  << std::endl;
	}
}

int main(int argc,char* argv[]){
	test1();
	test2();
	return 0;
}