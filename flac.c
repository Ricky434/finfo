#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "flac.h"
#include "utils.h"

/*
* Return a null terminated string representing the input flac metadata type.
*/
char *flac_metadata_type_str(enum flac_metadata_type type) {
	switch (type) {
	case FLAC_STREAMINFO_TYPE:
		return "STREAMINFO\0";
	case FLAC_PADDING_TYPE:
		return "PADDING\0";
	case FLAC_APPLICATION_TYPE:
		return "APPLICATIOn\0";
	case FLAC_SEEK_TABLE_TYPE:
		return "SEEK_TABLE\0";
	case FLAC_VORBIS_COMMENT_TYPE:
		return "VORBIS_COMMENT\0";
	case FLAC_CUESHEET_TYPE:
		return "CUESHEET\0";
	case FLAC_PICTURE_TYPE:
		return "PICTURE\0";
	default:
		return "UNKNOWN\0";
	}
}

void flac_print_streaminfo(struct flac_streaminfo *info) {
	printf("Min block size: %u\n", info->min_blk_size);
	printf("Max block size: %u\n", info->max_blk_size);
	printf("Min frame size: %u\n", info->min_frame_size);
	printf("Max frame size: %u\n", info->max_frame_size);
	printf("Sample rate: %u\n", info->sample_rate);
	printf("Number of channels: %u\n", info->channels + 1);
	printf("Bits per sample: %u\n", info->bits_per_sample + 1);
	printf("Total samples: %lu\n", info->interchannel_samples);
	printf("MD5sum:");
	for (int i = 0; i < 16; i++) {
		printf("%02x", (unsigned char)info->md5sum[i]);
	}
	printf("\n");
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a streaminfo metadata block,
* and put it inside DST.
*/
void flac_parse_streaminfo(unsigned char *block, int size,
						   struct flac_metadata_block *dst) {
	// First 16 bits
	dst->data.streaminfo.min_blk_size = BE_bytes_to_int(block, 2);
	// Next 16 bits
	dst->data.streaminfo.max_blk_size = BE_bytes_to_int(block + 2, 2);
	// Next 24 bits
	dst->data.streaminfo.min_frame_size = BE_bytes_to_int(block + 4, 3);
	// Next 24 bits
	dst->data.streaminfo.max_frame_size = BE_bytes_to_int(block + 7, 3);

	// Next 20 bits
	// We have to make this: 12345678 12345678 12340000
	// Into this:			 00001234 56781234 56781234
	unsigned char buf[3] = {
		((block[10] & 0b11110000) >> 4),
		((block[11] & 0b11110000) >> 4) + ((block[10] & 0b00001111) << 4),
		((block[12] & 0b11110000) >> 4) + ((block[11] & 0b00001111) << 4)};
	dst->data.streaminfo.sample_rate = BE_bytes_to_int(buf, 3);

	// Next 3 bits
	dst->data.streaminfo.channels = (block[12] & 0b00001110) >> 1;

	// Next 5 bits. The last bit of the 12th byte and the first 4 bits of the next byte.
	dst->data.streaminfo.bits_per_sample =
		((block[12] & 1) << 4) + ((block[13] & 0b11110000) >> 4);

	// Next 36 bits. The last 4 bits of the 13th byte and the next 4 bytes.
	uint64_t first4bits_sample_rate = block[13] & 0b00001111;
	dst->data.streaminfo.interchannel_samples =
		BE_bytes_to_int(block + 14, 4) + (first4bits_sample_rate << 8 * 4);

	// Last 128 bits (16 bytes).
	memcpy(dst->data.streaminfo.md5sum, block + 18, 16);

	flac_print_streaminfo(&dst->data.streaminfo);
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as an application metadata block,
* and put it inside DST.
*/
void flac_parse_application(unsigned char *block, int size,
							struct flac_metadata_block *dst) {
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a seek table metadata block,
* and put it inside DST.
*/
void flac_parse_seekTable(unsigned char *block, int size,
						  struct flac_metadata_block *dst) {
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a vorbis comment metadata block,
* and put it inside DST.
*/
void flac_parse_vorbisComment(unsigned char *block, int size,
							  struct flac_metadata_block *dst) {
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a cuesheet metadata block,
* and put it inside DST.
*/
void flac_parse_cuesheet(unsigned char *block, int size,
						 struct flac_metadata_block *dst) {
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a picture metadata block,
* and put it inside DST.
*/
void flac_parse_picture(unsigned char *block, int size,
						struct flac_metadata_block *dst) {
}

struct flac_metadata_block *flac_parse_block(unsigned char header[4],
											 FILE *file) {
	struct flac_metadata_block *block = malloc(sizeof(*block));

	// Block is the last one if the first bit of the first byte is 1.
	block->last_block = (header[0] & 0b10000000) >> 7;
	// The next 7 bits of the first byte codes for the block type.
	block->type = header[0] & 0b01111111;
	// The next 3 bytes code for the block length. NOTE: they are big endian I think.
	block->block_length = BE_bytes_to_int(&header[1], 3);

	//printf("last: %d, type: %s, length: %u\n", block->last_block, flac_metadata_type_str(block->type), block->block_length);
	printf("%02X:%02X:%02X:%02X, last: %d, type: %s, length: %u\n", header[0],
		   header[1], header[2], header[3], block->last_block,
		   flac_metadata_type_str(block->type), block->block_length);

	unsigned char *data = malloc(block->block_length);
	fread(data, block->block_length, 1, file);

	switch (block->type) {
	case FLAC_STREAMINFO_TYPE:
		flac_parse_streaminfo(data, block->block_length, block);
	case FLAC_PADDING_TYPE: {
		struct flac_padding padding = {.bytes = block->block_length};
		block->data.padding			= padding;
	}
	case FLAC_APPLICATION_TYPE:
		flac_parse_application(data, block->block_length, block);
	case FLAC_SEEK_TABLE_TYPE:
		flac_parse_seekTable(data, block->block_length, block);
	case FLAC_VORBIS_COMMENT_TYPE:
		flac_parse_vorbisComment(data, block->block_length, block);
	case FLAC_CUESHEET_TYPE:
		flac_parse_cuesheet(data, block->block_length, block);
	case FLAC_PICTURE_TYPE:
		flac_parse_picture(data, block->block_length, block);
	default:;
	}

	return block;
}

bool try_flac(FILE *file) {
	unsigned char signature[4];
	fread(signature, 4, 1, file);
	if (0x664C6143 != BE_bytes_to_int(signature, 4)) { return false; }

	while (true) {
		unsigned char header[4];
		fread(header, 4, 1, file);

		struct flac_metadata_block *block = flac_parse_block(header, file);
		if (block->last_block) break;
	}

	return true;
}
