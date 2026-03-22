#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "finfo_utils.h"

// TODO: Maybe make one for each int size?
// - maybe more efficient without loop
// - maybe easier to find bugs if wrong arguments passed
// nah
uint64_t BE_bytes_to_int(unsigned char *bytes, unsigned short len) {
	uint64_t res = 0;

	int actual_len = len;
	if (len > 8) { actual_len = 8; }

	//    0      1      2      3
	// <<8*3  <<8*2  <<8*1  <<8*0
	for (int i = 0; i < actual_len; i++) {
		res += (uint64_t)bytes[i] << 8 * (actual_len - 1 - i);
	}

	return res;
}

uint64_t LE_bytes_to_int(unsigned char *bytes, unsigned short len) {
	uint64_t res = 0;

	int actual_len = len;
	if (len > 8) { actual_len = 8; }

	//    0      1      2      3
	// <<8*0  <<8*1  <<8*2  <<8*3
	for (int i = 0; i < actual_len; i++) { res += (uint64_t)bytes[i] << (8 * i); }

	return res;
}

// Encodes a data array of len *len in base64 and returns it
// The length residing in *len is updated with the length of the b64 encoded string
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
//
// list list_new(size_t initial_size, size_t elem_size) {
// 	if (initial_size < 32) {
// 		initial_size = 32;
// 	}
//
// 	void *items = malloc(elem_size * initial_size);
// 	list l = { 0, initial_size, elem_size, items };
// 	return l;
// }
//
// void *list_get(list *l, size_t index) {
// 	if (index >= l->len) { return NULL; }
// 	return (char *)l->items + index * l->elem_size;
// }
//
// int list_put(list *l, size_t index, void *item) {
// 	if (index >= l->len) { return -1; }
// 	memcpy(l->items + index * l->elem_size, item, l->elem_size);
// 	return 0;
// }
//
// void list_append(list *l, void *item) {
// 	if (l->len >= l->capacity) {
// 		l->items = realloc(l->items, l->capacity*2 * l->elem_size); 
// 		if (!l->items) { 
// 			printf("out of memory!");
// 			exit(1);
// 		}
// 		l->capacity *= 2;
// 	}
//
// 	memcpy(l->items + l->len * l->elem_size, item, l->elem_size);
// 	l->len++;
// }
//
// void *list_pop(list *l, size_t index) {
// 	if (index >= l->len || l->len == 0) { return NULL; }
//
// 	void *item = malloc(l->elem_size);
// 	memcpy(item, l->items + index * l->elem_size, l->elem_size);
//
// 	for (size_t i=index; i<l->len-1; i++){
// 		void *pos = l->items + i * l->elem_size; 
// 		memcpy(pos, pos+l->elem_size, l->elem_size);
// 	}
// 	l->len--;
// 	return item;
// }
//
// void list_free(list *l) {
// 	free(l->items);
// }
