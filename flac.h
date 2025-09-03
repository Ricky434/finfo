#ifndef FINFO_FLAC
#define FINFO_FLAC

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

enum flac_metadata_type {
	FLAC_STREAMINFO_TYPE	 = 0,
	FLAC_PADDING_TYPE		 = 1,
	FLAC_APPLICATION_TYPE	 = 2,
	FLAC_SEEK_TABLE_TYPE	 = 3,
	FLAC_VORBIS_COMMENT_TYPE = 4,
	FLAC_CUESHEET_TYPE		 = 5,
	FLAC_PICTURE_TYPE		 = 6,
	FLAC_UNKNOWN_TYPE
};

/*
 * The streaminfo metadata block has information about the whole FLAC stream,
 * such as sample rate, number of samples, etc.
*/
struct flac_streaminfo {
	// Min block size (in samples) used in the stream excluding the last block.
	uint16_t min_blk_size;
	// Max block size (in samples) used in the stream.
	uint16_t max_blk_size;
	// Min frame size (in bytes) used in the stream. 0 means it is not known.
	uint32_t min_frame_size;
	// Max frame size (in bytes) used in the stream. 0 means it is not known.
	uint32_t max_frame_size;
	// Sample rate in Hz.
	uint32_t sample_rate;
	// Number of channels - 1.
	uint8_t channels;
	// Bits per sample - 1.
	uint8_t bits_per_sample;
	// Total number of interchannel samples in the stream. 0 means it is not known.
	uint64_t interchannel_samples;
	// MD5 checksum of the unencoded audio data.
	unsigned char md5sum[6];
};

struct flac_padding {
	// Number of 0 bytes of padding,
	// it is the same as the length contained in the block header.
	uint8_t bytes;
};

struct flac_application {
	// Application ID (registered in IANA registry)
	uint32_t app_id;
	// Application data. Its length is the length contained in the block header
	// minus the 32 bits of the application id.
	char *app_data;
};

struct flac_seek_point {
	// Sample number of the first sample in the target frame.
	uint64_t first_sample;
	// Offset (in bytes) from the first byte of the first frame header
	// to the first byte of the target frame's header.
	uint64_t offset;
	// Number of samples in the target frame.
	uint16_t samples_n;
};

struct flac_seek_table {
	// Number of seek points stored in the table.
	size_t seek_points_n;
	// Array of seek points
	struct flac_seek_point *seek_points;
};

// NOTE: that the 32-bit field lengths are coded little-endian as opposed to the usual big-endian coding of fixed-length integers in the rest of the FLAC format.
struct flac_vorbis_field {
	// Length of the data contained in the field.
	uint32_t length;
	// Field data. It consists of a name and the contents, separated by a '='.
	char *data; 
};

// NOTE: that the 32-bit field lengths are coded little-endian as opposed to the usual big-endian coding of fixed-length integers in the rest of the FLAC format.
struct flac_vorbis_comment {
	// Length of the vendor string. The vendor string is not terminated in any way.
	uint32_t vendor_string_len;
	// Vendor string.
	unsigned char *vendor_string; // NOTE: this is coded as UTF-8
	// Number of fields in the vorbis comment block.
	uint32_t fields_n;
	// Array of vorbis fields.
	struct flac_vorbis_field *fields;
};

struct flac_cuesheet {
};

struct flac_picture {
};

/*
 * Metadata block for the FLAC file type.
*/
struct flac_metadata_block {
	enum flac_metadata_type type;
	bool last_block;
	uint8_t block_length;

	union {
	} data;
};

bool try_flac(FILE *file);

#endif
