#ifndef FINFO_PNG_H
#define FINFO_PNG_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

extern unsigned char PNG_SIGNATURE[8];

struct png_chunk {
	// Length of the data field.
	uint32_t length;
	// Chunk type.
	char type[4];
	// Chunk data.
	unsigned char *data;
	// Cyclic Redundancy Code calculated on the chunk type and data.
	unsigned char CRC[4];
};

void png_chunk_free(struct png_chunk *chunk);

bool try_png(FILE *file);

void print_png_file(FILE *file);
void print_png(unsigned char *data, size_t data_len);

#endif // !FINFO_PNG_H
