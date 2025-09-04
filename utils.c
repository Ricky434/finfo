#include <stdint.h>

uint64_t BE_bytes_to_int(unsigned char *bytes, unsigned short len) {
	uint64_t res = 0;

	int actual_len = len;
	if (len > 8) { actual_len = 8; }

	//    0      1      2      3
	// <<8*3  <<8*2  <<8*1  <<8*0
	for (int i = 0; i < actual_len; i++) {
		res += bytes[i] << 8 * (len - 1 - i);
	}

	return res;
}

uint64_t LE_bytes_to_int(unsigned char *bytes, unsigned short len) {
	uint64_t res = 0;

	int actual_len = len;
	if (len > 8) { actual_len = 8; }

	//    0      1      2      3
	// <<8*0  <<8*1  <<8*2  <<8*3
	for (int i = 0; i < actual_len; i++) { res += bytes[i] << (8 * i); }

	return res;
}
