#ifndef FINFO_FLAC_H
#define FINFO_FLAC_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

extern unsigned char FLAC_SIGNATURE[4];

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

char *flac_metadata_type_str(enum flac_metadata_type type);

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
	unsigned char md5sum[16];
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
	unsigned char *app_data;
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

struct flac_cuesheet_track_idx_point {
	// Offset in samples relative to the track offset.
	uint64_t offset;
	// Track index point number.
	uint8_t number;
};

struct flac_cuesheet_track {
	// Track offset of the first index point in samples.
	uint64_t offset;
	// Track number.
	uint8_t number;
	// Track ISRC.
	char ISRC[12];
	// Track type. True for audio, false for non-audio.
	bool audio;
	// Pre-emphasis flag. True for pre-emphasis, false for no pre-emphasis.
	bool pre_emphasis;
	// Number of track index points.
	uint8_t idx_points_n;
	// Array of cuesheet track index points.
	struct flac_cuesheet_track_idx_point *idx_points;
};

struct flac_cuesheet {
	// Media catalog number.
	char catalog_number[128];
	// Number of lead-in samples.
	uint64_t leadin_samples;
	// True if the cuesheet corresponds to a CD-DA.
	bool cd_da;
	// Number of tracks in this cuesheet.
	uint8_t tracks_n;
	// Array of cuesheet tracks.
	struct flac_cuesheet_track *tracks;
};

enum flac_picture_type {
	FLAC_OTHER_PICTURE_TYPE					= 0,
	FLAC_PNG_ICON_PICTURE_TYPE				= 1,
	FLAC_GENERAL_ICON_PICTURE_TYPE			= 2,
	FLAC_FRONT_COVER_PICTURE_TYPE			= 3,
	FLAC_BACK_COVER_PICTURE_TYPE			= 4,
	FLAC_LINER_NOTES_PAGE_PICTURE_TYPE		= 5,
	FLAC_MEDIA_LABEL_PICTURE_TYPE			= 6,
	FLAC_LEAD_ARTIST_PICTURE_TYPE			= 7,
	FLAC_ARTIST_PICTURE_TYPE				= 8,
	FLAC_CONDUCTOR_PICTURE_TYPE				= 9,
	FLAC_BAND_PICTURE_TYPE					= 10,
	FLAC_COMPOSER_PICTURE_TYPE				= 11,
	FLAC_LYRICIST_PICTURE_TYPE				= 12,
	FLAC_RECORDING_LOCATION_PICTURE_TYPE	= 13,
	FLAC_DURING_RECORDING_PICTURE_TYPE		= 14,
	FLAC_DURING_PERFORMANCE_PICTURE_TYPE	= 15,
	FLAC_VIDEO_CAPTURE_PICTURE_TYPE			= 16,
	FLAC_A_BRIGHT_COLORED_FISH_PICTURE_TYPE = 17,
	FLAC_ILLUSTRATION_PICTURE_TYPE			= 18,
	FLAC_BAND_LOGO_PICTURE_TYPE				= 19,
	FLAC_PUBLISHER_LOGO_PICTURE_TYPE		= 20,
	FLAC_INVALID_PICTURE_TYPE
};

struct flac_picture {
	// Picture type.
	enum flac_picture_type type;
	// Length of the media type string in bytes.
	uint32_t media_type_string_len;
	// Media type string or the text string --> to signify that the data part is
	// a URI.
	char *media_type_string;
	// Length of the description string in bytes.
	uint32_t description_len;
	// Description of the picture.
	char *description; // NOTE: this is coded as UTF-8
	// Width of the picture in pixels.
	uint32_t picture_width;
	// Height of the picture in pixels.
	uint32_t picture_height;
	// Color depth of the picture in bits per pixel.
	uint32_t color_depth;
	// Number of colors used for indexed-color pictures.
	// This value is 0 for non-indexed pictures.
	uint32_t color_n;
	// Length of the picture data in bytes.
	uint32_t data_len;
	// Binary picture data.
	unsigned char *data;
};

/*
 * Metadata block for the FLAC file type.
*/
struct flac_metadata_block {
	enum flac_metadata_type type;
	bool last_block;
	uint32_t block_length;

	union {
		struct flac_streaminfo streaminfo;
		struct flac_padding padding;
		struct flac_application application;
		struct flac_seek_table seek_table;
		struct flac_vorbis_comment vorbis_comment;
		struct flac_cuesheet cuesheet;
		struct flac_picture picture;
	} data;
};

void flac_metadata_block_free(struct flac_metadata_block *block);

bool try_flac(FILE *file);

#endif // FINFO_FLAC_H
