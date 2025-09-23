#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
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

	// Reset position to start of file for printing it
	fseek(file, 0, SEEK_SET);
	print_png_file(file);

	return true;
}

#define KITTY_ESCAPE_START "\033_G"
#define KITTY_ESCAPE_END "\033\\"
#define KITTY_CHUNK_SIZE 4096

void print_png_file(FILE *file) {
	struct winsize sz;
    ioctl(0, TIOCGWINSZ, &sz);

	char control_codes[50];
	snprintf(control_codes, sizeof(control_codes), ",a=T,f=100,c=%d", sz.ws_col);

	unsigned char *buf	= malloc(KITTY_CHUNK_SIZE);
 // KITTY_CHUNK_SIZE of size 1, since fread returns how many items were read
	size_t read_n		= fread(
		  buf, 1, KITTY_CHUNK_SIZE,
		  file);
	while (read_n > 0) {
		char *encoded = base64_encode(buf, &read_n);

		int last = read_n < KITTY_CHUNK_SIZE;
		printf("%sm=%d%s;%.*s%s", KITTY_ESCAPE_START, !last, control_codes,
			   (int)read_n, encoded, KITTY_ESCAPE_END);

		// Control codes should be specified only in first chunk
		*control_codes = '\0';

		read_n = fread(buf, 1, KITTY_CHUNK_SIZE, file);
	}

	putchar('\n');
}

void print_png(unsigned char *data, size_t data_len) {
	struct winsize sz;
    ioctl(0, TIOCGWINSZ, &sz);

	char control_codes[50];
	snprintf(control_codes, sizeof(control_codes), ",a=T,f=100,c=%d", sz.ws_col);

	size_t read_data = 0;
	while (data_len > read_data) {
		size_t to_read = (data_len - read_data) < KITTY_CHUNK_SIZE
							 ? (data_len - read_data)
							 : KITTY_CHUNK_SIZE;

		size_t asd = to_read;
		char *encoded  = base64_encode(&data[read_data], &asd);

		int last = to_read < KITTY_CHUNK_SIZE;
		printf("%sm=%d%s;%.*s%s", KITTY_ESCAPE_START, !last, control_codes,
		   (int)asd, encoded, KITTY_ESCAPE_END);

		// Control codes should be specified only in first chunk
		*control_codes = '\0';

		read_data += to_read;
	}

	putchar('\n');
}
