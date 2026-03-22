#ifndef FINFO_UTILS_H
#define FINFO_UTILS_H

#include <stdint.h>
#include <stdlib.h>

// Convert a BigEndian byte array into an unsigned 64 bit int.
// Since 64 bits are 8 bytes, the max length of the array is 8.
uint64_t BE_bytes_to_int(unsigned char *bytes, unsigned short len);
// Convert a LittleEndian byte array into an unsigned 64 bit int.
// Since 64 bits are 8 bytes, the max length of the array is 8.
uint64_t LE_bytes_to_int(unsigned char *bytes, unsigned short len);
// Base64 encode len bytes of data. The len argument will be updated
// with the length of the returned string.
// Returns NULL if the provided len is 0.
char *base64_encode(unsigned char *data, size_t *len);

// //Bad implementation of a list
// typedef struct {
// 	size_t len;
// 	size_t capacity;
// 	size_t elem_size;
// 	void *items;
// } list;
//
// list list_new(size_t initial_size, size_t elem_size);
// void *list_get(list *l, size_t index);
// int list_put(list *l, size_t index, void *item);
// void list_append(list *l, void *item);
// void *list_pop(list *l, size_t index);
// void list_free(list *l);

#endif // !FINFO_UTILS_H
