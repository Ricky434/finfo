#ifndef FINFO_UTILS_H
#define FINFO_UTILS_H

#include <stdint.h>

// Convert a BigEndian byte array into an unsigned 64 bit int.
// Since 64 bits are 8 bytes, the max length of the array is 8.
uint64_t BE_bytes_to_int(unsigned char *bytes, unsigned short len);
// Convert a LittleEndian byte array into an unsigned 64 bit int.
// Since 64 bits are 8 bytes, the max length of the array is 8.
uint64_t LE_bytes_to_int(unsigned char *bytes, unsigned short len);

#endif // FINFO_UTILS_H
