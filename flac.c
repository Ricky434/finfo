#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>
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

// ===== Block printers =====

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

void flac_print_application(struct flac_application *application) {
	// TODO:test
	printf("AppId: %d, App data: %s\n", application->app_id,
		   application->app_data);
}

void flac_print_seek_table(struct flac_seek_table *table) {
	for (size_t i = 0; i < table->seek_points_n; i++) {
		struct flac_seek_point point = table->seek_points[i];
		printf("first sample: %lu, offset: %lu, samples: %ud\n",
			   point.first_sample, point.offset, point.samples_n);
	}
}

void flac_print_vorbis_comment(struct flac_vorbis_comment *vorbis) {
	printf("vendor: %.*s\n", vorbis->vendor_string_len, vorbis->vendor_string);

	for (size_t i = 0; i < vorbis->fields_n; i++) {
		printf("%.*s\n", vorbis->fields[i].length, vorbis->fields[i].data);
	}
}

void flac_print_cuesheet(struct flac_cuesheet *cuesheet) {
	printf("Media catalog number: %.128s\n", cuesheet->catalog_number);
	printf("Lead-in samples: %lu\n", cuesheet->leadin_samples);
	printf("CD-DA: %d\n", cuesheet->cd_da);
	printf("Number of tracks: %u\n", cuesheet->tracks_n);

	for (size_t i = 0; i < cuesheet->tracks_n; i++) {
		printf("Track %lu\n", i);
		printf("\tOffset: %lu\n", cuesheet->tracks[i].offset);
		printf("\tNumber: %u\n", cuesheet->tracks[i].number);
		printf("\tISRC: %.12s\n", cuesheet->tracks[i].ISRC);
		printf("\tAudio: %d\n", cuesheet->tracks[i].audio);
		printf("\tPre-emphasis: %d\n", cuesheet->tracks[i].pre_emphasis);
		printf("\tNumber of index points: %u\n",
			   cuesheet->tracks[i].idx_points_n);

		for (size_t j = 0; j < cuesheet->tracks[i].idx_points_n; j++) {
			printf("\tPoint %lu\n", j);
			printf("\t\tOffset: %lu\n",
				   cuesheet->tracks[i].idx_points[j].offset);
			printf("\t\tNumber: %u\n",
				   cuesheet->tracks[i].idx_points[j].number);
		}
	}
}

void flac_print_picture(struct flac_picture *picture) {
	printf("Picture type: %u\n", picture->type);
	printf("Media type strlen: %u\n", picture->media_type_string_len);
	printf("Media type: %s\n", picture->media_type_string);
	printf("Description strlen: %u\n", picture->description_len);
	printf("Description: %s\n", picture->description);
	printf("Color depth: %u\n", picture->color_depth);
	printf("Number of colors: %u\n", picture->color_n);
	printf("Picture width: %u\n", picture->picture_width);
	printf("Picture height: %u\n", picture->picture_height);
	printf("Data len: %u\n", picture->data_len);

	// TODO: check errors
	FILE *picture_file = fopen("./temp_picture", "wb");
	fwrite(picture->data, picture->data_len, 1, picture_file);
	fclose(picture_file);

	// TODO: print image to terminal with kitty
}

// ===== Block parsers =====

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a streaminfo metadata block,
* and put it inside DST.
*/
void flac_parse_streaminfo(unsigned char *block, int size,
						   struct flac_metadata_block *dst) {
	struct flac_streaminfo *streaminfo = &dst->data.streaminfo;
	// First 16 bits
	streaminfo->min_blk_size = BE_bytes_to_int(block, 2);
	// Next 16 bits
	streaminfo->max_blk_size = BE_bytes_to_int(block + 2, 2);
	// Next 24 bits
	streaminfo->min_frame_size = BE_bytes_to_int(block + 4, 3);
	// Next 24 bits
	streaminfo->max_frame_size = BE_bytes_to_int(block + 7, 3);

	// Next 20 bits
	// We have to make this: 12345678 12345678 12340000
	// Into this:			 00001234 56781234 56781234
	unsigned char buf[3] = {
		((block[10] & 0b11110000) >> 4),
		((block[11] & 0b11110000) >> 4) + ((block[10] & 0b00001111) << 4),
		((block[12] & 0b11110000) >> 4) + ((block[11] & 0b00001111) << 4)};
	streaminfo->sample_rate = BE_bytes_to_int(buf, 3);

	// Next 3 bits
	streaminfo->channels = (block[12] & 0b00001110) >> 1;

	// Next 5 bits. The last bit of the 12th byte and the first 4 bits of the next byte.
	streaminfo->bits_per_sample =
		((block[12] & 1) << 4) + ((block[13] & 0b11110000) >> 4);

	// Next 36 bits. The last 4 bits of the 13th byte and the next 4 bytes.
	uint64_t first4bits_sample_rate = block[13] & 0b00001111;
	streaminfo->interchannel_samples =
		BE_bytes_to_int(block + 14, 4) + (first4bits_sample_rate << 8 * 4);

	// Last 128 bits (16 bytes).
	memcpy(streaminfo->md5sum, block + 18, 16);

	flac_print_streaminfo(streaminfo);
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as an application metadata block,
* and put it inside DST.
*/
void flac_parse_application(unsigned char *block, int size,
							struct flac_metadata_block *dst) {
	struct flac_application *application = &dst->data.application;

	application->app_id	  = BE_bytes_to_int(block, 4);
	application->app_data = malloc(dst->block_length - 4);
	memcpy(application->app_data, block + 4, dst->block_length - 4);

	flac_print_application(application);
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a seek table metadata block,
* and put it inside DST.
*/
void flac_parse_seekTable(unsigned char *block, int size,
						  struct flac_metadata_block *dst) {
	struct flac_seek_table *seek_table = &dst->data.seek_table;

	// Each seek point is 18 bytes long.
	const int seek_point_size = 18;
	size_t points			  = dst->block_length / seek_point_size;

	seek_table->seek_points_n = points;
	seek_table->seek_points	  = calloc(points, sizeof(struct flac_seek_point));

	for (size_t i = 0; i < points; i++) {
		unsigned char *point_p = block + (i * seek_point_size);

		struct flac_seek_point point = {
			// First 64 bits of the point.
			.first_sample = BE_bytes_to_int(point_p, 8),
			// Next 64 bits of the point.
			.offset = BE_bytes_to_int(point_p, 8),
			// Last 16 bits of the point.
			.samples_n = BE_bytes_to_int(point_p, 2),
		};

		seek_table->seek_points[i] = point;
	}

	flac_print_seek_table(seek_table);
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a vorbis comment metadata block,
* and put it inside DST.
*/
void flac_parse_vorbisComment(unsigned char *block, int size,
							  struct flac_metadata_block *dst) {
	struct flac_vorbis_comment *vorbis = &dst->data.vorbis_comment;

	vorbis->vendor_string_len = LE_bytes_to_int(block, 4);

	vorbis->vendor_string = malloc(vorbis->vendor_string_len);
	memcpy(vorbis->vendor_string, block + 4, vorbis->vendor_string_len);

	vorbis->fields_n =
		LE_bytes_to_int(block + 4 + vorbis->vendor_string_len, 4);

	vorbis->fields = calloc(vorbis->fields_n, sizeof(struct flac_vorbis_field));

	// The fields start after the vendor string length,
	// the vendor string, and the fields number.
	unsigned char *fields_start = block + 4 + vorbis->vendor_string_len + 4;

	int offset = 0;
	for (size_t i = 0; i < vorbis->fields_n; i++) {
		vorbis->fields[i].length = LE_bytes_to_int(fields_start + (offset), 4);

		vorbis->fields[i].data = malloc(vorbis->fields[i].length);
		memcpy(vorbis->fields[i].data, fields_start + offset + 4,
			   vorbis->fields[i].length);

		// Update the offset from the start of the vorbis fields by
		// adding the bytes occupied by the current field's
		// length and data.
		offset += 4 + vorbis->fields[i].length;
	}

	flac_print_vorbis_comment(vorbis);
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a cuesheet metadata block,
* and put it inside DST.
*/
void flac_parse_cuesheet(unsigned char *block, int size,
						 struct flac_metadata_block *dst) {
	struct flac_cuesheet *cuesheet = &dst->data.cuesheet;

	memcpy(cuesheet->catalog_number, block, 128);
	cuesheet->leadin_samples = BE_bytes_to_int(block + 128, 8);
	cuesheet->cd_da			 = (block[136] & 0b10000000) >> 7;
	// 258 reserved bytes.
	unsigned char *tracks_start = block + 137 + 258;
	cuesheet->tracks_n			= BE_bytes_to_int(tracks_start, 1);
	cuesheet->tracks =
		calloc(cuesheet->tracks_n, sizeof(struct flac_cuesheet_track));

	unsigned char *current_track_start = tracks_start + 1;
	for (int i = 0; i < cuesheet->tracks_n; i++) {
		struct flac_cuesheet_track *track = &cuesheet->tracks[i];
		track->offset = BE_bytes_to_int(current_track_start, 8);
		track->number = BE_bytes_to_int(current_track_start + 8, 1);
		memcpy(track->ISRC, current_track_start + 9, 12);
		track->audio		= !((current_track_start[21] & 0b10000000) >> 7);
		track->pre_emphasis = (current_track_start[21] & 0b01000000) >> 6;
		// 13 reserved bytes.
		unsigned char *points_start = current_track_start + 22 + 13;
		track->idx_points_n = BE_bytes_to_int(points_start, 1);
		track->idx_points	= calloc(
			  track->idx_points_n, sizeof(struct flac_cuesheet_track_idx_point));

		unsigned char *curr_idx_point = points_start + 1;
		for (int j = 0; j < track->idx_points_n; j++) {
			track->idx_points[i].offset = BE_bytes_to_int(curr_idx_point, 8);
			track->idx_points[i].number =
				BE_bytes_to_int(curr_idx_point + 8, 1);
			// 3 reserved bytes.
			curr_idx_point = curr_idx_point + 9 + 3;
		}

		current_track_start = curr_idx_point;
	}

	flac_print_cuesheet(cuesheet);
}

/*
* Parse the given array of bytes BLOCK, long SIZE bytes, as a picture metadata block,
* and put it inside DST.
*/
void flac_parse_picture(unsigned char *block, int size,
						struct flac_metadata_block *dst) {
	struct flac_picture *picture = &dst->data.picture;

	picture->type				   = BE_bytes_to_int(block, 4);
	picture->media_type_string_len = BE_bytes_to_int(block + 4, 4);
	picture->media_type_string =
		calloc(picture->media_type_string_len, sizeof(char));
	memcpy(picture->media_type_string, block + 8,
		   picture->media_type_string_len);

	unsigned char *descr_start = block + 8 + picture->media_type_string_len;
	picture->description_len   = BE_bytes_to_int(descr_start, 4);
	picture->description	   = calloc(picture->description_len, sizeof(char));
	memcpy(picture->description, descr_start + 4, picture->description_len);

	unsigned char *descr_end = descr_start + 4 + picture->description_len;
	picture->picture_width	 = BE_bytes_to_int(descr_end, 4);
	picture->picture_height	 = BE_bytes_to_int(descr_end + 4, 4);
	picture->color_depth	 = BE_bytes_to_int(descr_end + 8, 4);
	picture->color_n		 = BE_bytes_to_int(descr_end + 12, 4);

	picture->data_len = BE_bytes_to_int(descr_end + 16, 4);
	picture->data	  = calloc(picture->data_len, sizeof(*picture->data));
	memcpy(picture->data, descr_end + 20, picture->data_len);

	flac_print_picture(picture);
}

// ===== Block functions =====

struct flac_metadata_block *flac_parse_block(unsigned char header[4],
											 FILE *file) {
	struct flac_metadata_block *block = malloc(sizeof(*block));

	// Block is the last one if the first bit of the first byte is 1.
	block->last_block = (header[0] & 0b10000000) >> 7;
	// The next 7 bits of the first byte codes for the block type.
	block->type = header[0] & 0b01111111;
	// The next 3 bytes code for the block length.
	block->block_length = BE_bytes_to_int(&header[1], 3);

	printf("%02X:%02X:%02X:%02X, last: %d, type: %s, length: %u\n", header[0],
		   header[1], header[2], header[3], block->last_block,
		   flac_metadata_type_str(block->type), block->block_length);

	unsigned char *data = malloc(block->block_length);
	fread(data, block->block_length, 1, file);

	switch (block->type) {
	case FLAC_STREAMINFO_TYPE:
		flac_parse_streaminfo(data, block->block_length, block);
		break;
	case FLAC_PADDING_TYPE: {
		struct flac_padding padding = {.bytes = block->block_length};
		block->data.padding			= padding;
		break;
	}
	case FLAC_APPLICATION_TYPE:
		flac_parse_application(data, block->block_length, block);
		break;
	case FLAC_SEEK_TABLE_TYPE:
		flac_parse_seekTable(data, block->block_length, block);
		break;
	case FLAC_VORBIS_COMMENT_TYPE:
		flac_parse_vorbisComment(data, block->block_length, block);
		break;
	case FLAC_CUESHEET_TYPE:
		flac_parse_cuesheet(data, block->block_length, block);
		break;
	case FLAC_PICTURE_TYPE:
		flac_parse_picture(data, block->block_length, block);
		break;
	case FLAC_UNKNOWN_TYPE:
		break;
	}

	free(data);
	return block;
}

void flac_metadata_block_free(struct flac_metadata_block *block) {
	switch (block->type) {
	case FLAC_STREAMINFO_TYPE:
	case FLAC_PADDING_TYPE:
	case FLAC_UNKNOWN_TYPE:
		break;
	case FLAC_APPLICATION_TYPE:
		free(block->data.application.app_data);
		break;
	case FLAC_SEEK_TABLE_TYPE:
		free(block->data.seek_table.seek_points);
		break;
	case FLAC_VORBIS_COMMENT_TYPE:
		free(block->data.vorbis_comment.vendor_string);
		for (int i = 0; i < block->data.vorbis_comment.fields_n; i++) {
			free(block->data.vorbis_comment.fields[i].data);
		}
		free(block->data.vorbis_comment.fields);
		break;
	case FLAC_CUESHEET_TYPE:
		for (int i = 0; i < block->data.cuesheet.tracks_n; i++) {
			free(block->data.cuesheet.tracks[i].idx_points);
		}
		free(block->data.cuesheet.tracks);
		break;
	case FLAC_PICTURE_TYPE:
		free(block->data.picture.media_type_string);
		free(block->data.picture.description);
		free(block->data.picture.data);
		break;
	}

	free(block);
}

bool try_flac(FILE *file) {
	unsigned char signature[4];
	fread(signature, 4, 1, file);
	if (0x664C6143 != BE_bytes_to_int(signature, 4)) { return false; }

	while (true) {
		unsigned char header[4];
		fread(header, 4, 1, file);

		struct flac_metadata_block *block = flac_parse_block(header, file);
		if (block->last_block) {
			flac_metadata_block_free(block);
			break;
		}
		flac_metadata_block_free(block);
	}

	return true;
}
