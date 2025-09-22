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

#endif // !FINFO_UTILS_H
