#include "base58.h"

static const char b58digits_ordered[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static const int8_t b58digits_map[] = {
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
	-1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
	22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
	-1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
	47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
};

/**
 * encode an array of bytes into a base58 string
 * @param binary_data the data to be encoded
 * @param binary_data_size the size of the data to be encoded
 * @param base58 the results buffer
 * @param base58_size the size of the results buffer
 * @returns true(1) on success
 */
int xdag_base58_encode(const unsigned char* data, size_t binsz, unsigned char** b58, size_t* b58sz)
{
	const uint8_t* bin = data;
	int carry;
	ssize_t i, j, high, zcount = 0;
	size_t size;
	
	while (zcount < (ssize_t)binsz && !bin[zcount]) {
		++zcount;
	}
	
	size = (binsz - zcount) * 138 / 100 + 1;
	uint8_t buf[size];
	memset(buf, 0, size);
	
	for (i = zcount, high = size - 1; i < (ssize_t)binsz; ++i, high = j) {
		for (carry = bin[i], j = size - 1; (j > high) || carry; --j) {
			carry += 256 * buf[j];
			buf[j] = carry % 58;
			carry /= 58;
		}
	}
	
	for (j = 0; j < (ssize_t)size && !buf[j]; ++j)
		;
	
	if (*b58sz <= zcount + size - j) {
		*b58sz = zcount + size - j + 1;
		memset(buf, 0, size);
		return 0;
	}
	
	if (zcount) {
		memset(b58, '1', zcount);
	}
	for (i = zcount; j < (ssize_t)size; ++i, ++j) {
		(*b58)[i] = b58digits_ordered[buf[j]];
	}
	(*b58)[i] = '\0';
	*b58sz = i + 1;
	
	memset(buf, 0, size);
	return 1;
}