#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "finfo_utils.h"

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

char *base64_encode(unsigned char *data, size_t *len) {
	if (*len == 0) {
		return NULL;
	}

	const char *b64_table =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	size_t new_len = ((*len / 3) + (*len % 3  > 0)) * 4;
	char *encoded = malloc(new_len);

	int data_p = 0;
	int enc_p = 0;

	// TODO: make it better

	while (*len - data_p >= 3) {
		encoded[enc_p+0] = b64_table[data[data_p+0] >> 2];
		encoded[enc_p+1] = b64_table[(data[data_p+1] >> 4) | ((data[data_p+0] & 0b11) << 4)];
		encoded[enc_p+2] = b64_table[(data[data_p+2] >> 6) | ((data[data_p+1] & 0b1111) << 2)];
		encoded[enc_p+3] = b64_table[data[data_p+2] & 0b111111];

		data_p+=3;
		enc_p+=4;
	}

	int remaining = *len % 3;
	if (remaining == 1) {
		encoded[enc_p+0] = b64_table[data[data_p+0] >> 2];
		encoded[enc_p+1] = b64_table[(data[data_p+0] & 0b11) << 4];
		encoded[enc_p+2] = '=';
		encoded[enc_p+3] = '=';
	} else if (remaining == 2) {
		encoded[enc_p+0] = b64_table[data[data_p+0] >> 2];
		encoded[enc_p+1] = b64_table[(data[data_p+1] >> 4) | ((data[data_p+0] & 0b11) << 4)];
		encoded[enc_p+2] = b64_table[(data[data_p+1] & 0b1111) << 2];
		encoded[enc_p+3] = '=';
	}

	*len = new_len;
	return encoded;
}
