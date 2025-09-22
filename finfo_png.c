#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "finfo_png.h"
#include "finfo_utils.h"

unsigned char PNG_SIGNATURE[8] = {'\x89', '\x50', '\x4E', '\x47',
								  '\x0D', '\x0A', '\x1A', '\x0A'};

void png_chunk_free(struct png_chunk *chunk) {
	free(chunk->data);
	free(chunk);
}

bool png_chunk_is_critical(struct png_chunk *ch) {
	// Bit 5 of first byte of type equals 1 if chunk is
	// ancillary, 0 if it is critical.
	return !((ch->type[0] & 0b00100000) >> 5);
}

bool png_chunk_is_private(struct png_chunk *ch) {
	// Bit 5 of second byte of type equals 1 if chunk is
	// private.
	return (ch->type[1] & 0b00100000) >> 5;
}

bool png_chunk_is_safe_to_copy(struct png_chunk *ch) {
	// Bit 5 of third byte of type equals 1 if chunk is
	// safe to copy.
	return (ch->type[3] & 0b00100000) >> 5;
}

struct png_chunk *png_parse_chunk(FILE *file) {
	struct png_chunk *chunk = malloc(sizeof(*chunk));

	unsigned char len_btyes[4];
	fread(len_btyes, 4, 1, file);
	chunk->length = BE_bytes_to_int(len_btyes, 4);

	fread(chunk->type, 4, 1, file);

	chunk->data = malloc(chunk->length);
	fread(chunk->data, chunk->length, 1, file);

	fread(chunk->CRC, 4, 1, file);

	return chunk;
}

void png_print_chunk(struct png_chunk *chunk) {
	printf("%.4s, length: %d\n", chunk->type, chunk->length);
}

bool try_png(FILE *file) {
	printf("Trying png...\n");
	unsigned char signature[8];
	fread(signature, 8, 1, file);
	if (memcmp(signature, PNG_SIGNATURE, 8)) { return false; }

	int data_count = 0;
	while (true) {
		struct png_chunk *chunk = png_parse_chunk(file);

		bool is_data = strncmp(chunk->type, "IDATA", 4) == 0;
		if (!is_data || !data_count++) { png_print_chunk(chunk); }

		if (strncmp(chunk->type, "IEND", 4) == 0) {
			printf("Total data chunks: %d\n", data_count);
			png_chunk_free(chunk);
			break;
		}

		png_chunk_free(chunk);
	}
	
	print_png(file);

	return true;
}

void print_png(FILE *file) {
#define	KITTY_ESCAPE_START "\033_G"
#define KITTY_ESCAPE_END "\033\\"

	char *test = malloc(11);
	test = "provaprova12";
	size_t test_len = 10;
	base64_encode((unsigned char *)test, &test_len);
}
