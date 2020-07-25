#include <iostream>
#include <sstream>
#include "../../client/rx_hashp.h"

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
	std::string seed="hello rx";

	for(int i=0;i < 1000;++i){
		uint8_t buf[32]={0};
		char template_buf[128]={0};
		sprintf(template_buf,"hello world %d",i);
		rx_slow_hashp(0, 0, seed.c_str(), template_buf, strlen(template_buf), buf, 4, 0);
		std::cout << time(nullptr) << " get rx slow hash " << hash2hex(buf)  << std::endl;
	}
}

void test2(){
	std::string seed1="hello rx 1";
	std::string seed2="hello rx 33333";

	std::string str1="hello world 1";

	uint8_t buf[32]={0};
	rx_slow_hashp(10000, 1000, seed1.c_str(), str1.c_str(), strlen(str1.c_str()), buf, 4, 0);
	std::cout << time(nullptr) << " rx slow hash with seed1 alt 0 " << hash2hex(buf)  << std::endl;

	rx_slow_hashp(10000, 1000, seed2.c_str(), str1.c_str(), strlen(str1.c_str()), buf, 4, 0);
	std::cout << time(nullptr) << " rx slow hash with seed2 alt 0 " << hash2hex(buf)  << std::endl;

//	rx_slow_hashp(10000,1000,seed1.c_str(),str1.c_str(),strlen(str1.c_str()),buf,4,1);
//	std::cout << time(nullptr) << " get rx slow hash with seed1 alt 1 " << hash2hex(buf)  << std::endl;
//
//	rx_slow_hashp(10000,1000,seed2.c_str(),str1.c_str(),strlen(str1.c_str()),buf,4,1);
//	std::cout << time(nullptr) << " get rx slow hash with seed2 alt 1 " << hash2hex(buf)  << std::endl;

}

int main(int argc,char* argv[]){
	test2();
	return 0;
}