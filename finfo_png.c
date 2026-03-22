#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "finfo_png.h"
#include "finfo_utils.h"
#include "finfo_kitty.h"

unsigned char PNG_SIGNATURE[8] = {'\x89', '\x50', '\x4E', '\x47',
								  '\x0D', '\x0A', '\x1A', '\x0A'};

bool png_chunk_is_critical(struct png_chunk *ch) {
	// Bit 5 of first byte of type equals 1 if chunk is
	// ancillary, 0 if it is critical.
	return !((ch->type_str[0] & 0b00100000) >> 5);
}

bool png_chunk_is_private(struct png_chunk *ch) {
	// Bit 5 of second byte of type equals 1 if chunk is
	// private.
	return (ch->type_str[1] & 0b00100000) >> 5;
}

bool png_chunk_is_safe_to_copy(struct png_chunk *ch) {
	// Bit 5 of third byte of type equals 1 if chunk is
	// safe to copy.
	return (ch->type_str[3] & 0b00100000) >> 5;
}

enum png_chunk_type png_parse_type(char type_str[4]);

// ===== Chunk printers =====
void png_print_IHDR(struct png_IHDR_chunk *ch) {
	printf("Width: %u\n", ch->width);
	printf("Height: %u\n", ch->height);
	printf("Bit depth: %u\n", ch->bit_depth);
	printf("Color type: %u\n", ch->color_type);
	printf("Compression method: %u\n", ch->compression_method);
	printf("Filter method: %u\n", ch->filter_method);
	printf("Interlace method: %u\n", ch->interlace_method);
}

void png_print_PLTE(struct png_PLTE_chunk *ch) {
	printf("Palette:\n");

	for (size_t i = 0; i<ch->palette_len; i++) {
		printf("\t%zu. R:%u, G:%u, B:%u\n", i, ch->palette[i].r, ch->palette[i].g, ch->palette[i].b);
	}
}

void png_print_IDAT(struct png_IDAT_chunk *ch) {

}

void png_print_chunk_type(struct png_chunk *chunk) {
	printf("----\n%.4s, length: %d\n", chunk->type_str, chunk->length);
}

// ===== Chunk parsers =====

/*
 * Parses the given byte array as IHDR png chunk data,
 * saving it in the provided chunk.
 */
int png_parse_IHDR(unsigned char *data, size_t size, struct png_chunk *ch) {
	if (size < PNG_IHDR_LEN) {
		fprintf(stderr, "Failed to parse IHDR chunk of length (%zul), less than required: %d.\n", size, PNG_IHDR_LEN);
		return -1;
	}

	ch->data.IHDR.width = BE_bytes_to_int(data, 4);
	ch->data.IHDR.height = BE_bytes_to_int(data+4, 4);
	ch->data.IHDR.bit_depth = *(data+8);
	ch->data.IHDR.color_type = *(data+9);
	ch->data.IHDR.compression_method = *(data+10);
	ch->data.IHDR.filter_method = *(data+11);
	ch->data.IHDR.interlace_method = *(data+12);
	
	return 0;
}

/*
 * Parses the given byte array as a PLTE png chunk data,
 * saving it in the provided chunk.
 */
int png_parse_PLTE(unsigned char *data, size_t size, struct png_chunk *ch) {
	if (size%3 != 0) { 
		fprintf(stderr, "Failed to parse PLTE chunk of length (%zul), not divisible by 3.\n", size);
		return -1; 
	}

	size_t palette_len = size/3;
	ch->data.PLTE.palette_len = palette_len;
	ch->data.PLTE.palette = malloc(palette_len * sizeof(*ch->data.PLTE.palette));
	for (int i = 0; i<palette_len; i++) {
		ch->data.PLTE.palette[i].r = data[i*3];
		ch->data.PLTE.palette[i].g = data[i*3+1];
		ch->data.PLTE.palette[i].b = data[i*3+2];
	}

	return 0;
}

/*
 * Parses the given byte array as a IDAT png chunk data,
 * saving it in the provided chunk.
 */
int png_parse_IDAT(unsigned char *data, size_t size, struct png_chunk *ch) {
	ch->data.IDAT.data = data;
	ch->data.IDAT.length = size;
	return 0;
}

// ===== ===== 

enum png_chunk_type png_parse_type(char type_str[4]) {
	return (enum png_chunk_type)BE_bytes_to_int((unsigned char *)type_str, 4);
}

// Parse the next PNG metadata chunk.
// The function assumes the file has been read up to the end of the
// previous chunk (or the PNG signature), and will read up to the end of the parsed chunk.
struct png_chunk *png_parse_chunk(FILE *file) {
	struct png_chunk *chunk = malloc(sizeof(*chunk));

	unsigned char len_btyes[4];
	fread(len_btyes, 4, 1, file);
	// TODO: check if fread succeeded: if (fread(len_bytes, 4, 1, file) != 1) free, return NULL;
	chunk->length = BE_bytes_to_int(len_btyes, 4);

	fread(chunk->type_str, 4, 1, file);

	unsigned char *data_buf = malloc(chunk->length); // NOTE: Risky if length is big
	fread(data_buf, chunk->length, 1, file);

	// TODO: text, itxt, exif, time
	int result = 0;
	switch (png_parse_type(chunk->type_str)) {
	case PLTE:
		result = png_parse_PLTE(data_buf, chunk->length, chunk);
		break;
	case IDAT:
		result = png_parse_IDAT(data_buf, chunk->length, chunk);
		break;
	case IHDR:
		result = png_parse_IHDR(data_buf, chunk->length, chunk);
		break;
	case IEND:
		break;
	case UNKNOWN:
		chunk->data.placeholder.data = data_buf;
		break;
	}

	fread(chunk->CRC, 4, 1, file);

	if (result != 0) {
		// If a parse function returned -1, we assume that it cleaned up after
		// itself and freed any memory it allocated already.
		free(chunk);
		return NULL;
	}

	return chunk;
}

void png_chunk_free(struct png_chunk *chunk) {
	switch (png_parse_type(chunk->type_str)) {
	case IHDR:
	case IEND:
		break;
	case PLTE:
		free(chunk->data.PLTE.palette);
		break;
	case IDAT:
		free(chunk->data.IDAT.data);
		break;
	case UNKNOWN:
		free(chunk->data.placeholder.data);
		break;
	}
	free(chunk);
}

void png_print_chunk(struct png_chunk *chunk) {
	png_print_chunk_type(chunk);

	switch (png_parse_type(chunk->type_str)) {
	case IHDR:
		png_print_IHDR(&chunk->data.IHDR);
		break;
	case IEND:
		break;
	case PLTE:
		png_print_PLTE(&chunk->data.PLTE);
		break;
	case IDAT:
		png_print_IDAT(&chunk->data.IDAT);
		break;
	case UNKNOWN:
		break;
	}
}

bool try_png(FILE *file) {
	printf("Trying png...\n");
	unsigned char signature[8];
	fread(signature, 8, 1, file);
	if (memcmp(signature, PNG_SIGNATURE, 8)) { return false; }

	int data_count = 0;
	while (true) {
		struct png_chunk *chunk	 = png_parse_chunk(file);
		if (!chunk) {
			fprintf(stderr, "Error parsing png, quitting.\n");
			return false;
		}
		enum png_chunk_type type = png_parse_type(chunk->type_str);

		if (type != IDAT || !data_count++) { png_print_chunk(chunk); }

		if (type == IEND) {
			printf("Total data chunks: %d\n", data_count);
			png_chunk_free(chunk);
			break;
		}

		png_chunk_free(chunk);
	}

	// Reset position to start of file for printing it
	fseek(file, 0, SEEK_SET);
	kitty_print_file(file);

	return true;
}

// Saves the width and height of an image file to ret_width and ret_height.
// It makes sure that the file STREAM is reset to the beginning after the function
// completes.
int png_get_image_sizes(FILE *file, uint32_t *ret_width, uint32_t *ret_height) {
	fseek(file, 0, SEEK_SET); // TODO: check error
	
	unsigned char signature[8];
	fread(signature, 8, 1, file);
	if (memcmp(signature, PNG_SIGNATURE, 8)) { 
		fprintf(stderr, "Not a PNG.\n");
		goto cleanup_err; 
	}

	struct png_chunk *chunk	 = png_parse_chunk(file);
	if (!chunk) {
		fprintf(stderr, "Error parsing, quitting.\n");
		goto cleanup_err;
	}
	enum png_chunk_type type = png_parse_type(chunk->type_str);

	if (type == IHDR) { 
		*ret_width = chunk->data.IHDR.width;
		*ret_height = chunk->data.IHDR.height;
	}
	png_chunk_free(chunk);
	fseek(file, 0, SEEK_SET); // TODO: check error
	return 0;
	
cleanup_err:
	fseek(file, 0, SEEK_SET); // TODO: check error
	return -1;
}
