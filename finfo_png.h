#ifndef FINFO_PNG_H
#define FINFO_PNG_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

extern unsigned char PNG_SIGNATURE[8];

/*
 * Types of PNG chunk.
 * The value associated is the int representation of the 4 bytes
 * read in network order.
 */
enum png_chunk_type {
	IHDR	= 0x49484452,
	PLTE	= 0x504C5445,
	IDAT	= 0x49444154,
	IEND	= 0x49454E44,
	UNKNOWN = 0,
};

// ===== Chunks =====

struct png_IHDR_chunk {
	uint32_t width;
	uint32_t height;
	unsigned char bit_depth;
	unsigned char color_type;
	unsigned char compression_method;
	unsigned char filter_method;
	unsigned char interlace_method;
};

struct png_PLTE_chunk {
	struct {
		unsigned char r;
		unsigned char g;
		unsigned char b;
	} *palette;
	uint32_t palette_len;
};

struct png_IDAT_chunk {
	unsigned char *data;
};

struct png_IEND_chunk {};

// ===== =====

struct png_chunk {
	// Length of the data field.
	uint32_t length;
	// Chunk type.
	char type_str[4];
	// Chunk data.
	union {
		struct png_IHDR_chunk IHDR;
		struct png_PLTE_chunk PLTE;
		struct png_IDAT_chunk IDAT;
		struct png_IEND_chunk IEND;
		struct png_IDAT_chunk placeholder;
	} data;
	// Cyclic Redundancy Code calculated on the chunk type and data.
	unsigned char CRC[4];
};

void png_chunk_free(struct png_chunk *chunk);

bool try_png(FILE *file);

void print_png_file(FILE *file);
void print_png(unsigned char *data, size_t data_len);

#endif // !FINFO_PNG_H
