#ifndef FINFO_JPEG_H
#define FINFO_JPEG_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define JPEG_MARKER_START 0xFF // First byte of marker

#define JPEG_MARKER_RESTART_MIN 0xD0
#define JPEG_MARKER_RESTART_MAX 0xD7

enum jpeg_marker_id {
	JPEG_MARKER_SOI	 = 0xD8, // Start of Image
	JPEG_MARKER_EOI  = 0xD9, // End of Image terminator
	JPEG_MARKER_SOS  = 0xDA, // Start of Scan
	JPEG_MARKER_DQT	 = 0xDB, // Define Quantization Table
	JPEG_MARKER_SOF0 = 0xC0, // Start of frame (baseline jpeg)
	JPEG_MARKER_DHT  = 0xC4 // Define Huffman Tables
};

struct jpeg_frame_component {
	uint8_t id;
	uint8_t horizontal_sampling_factor;
	uint8_t vertical_sampling_factor;
	uint8_t destination;
};

struct jpeg_SOF_segment {
	uint8_t precision;
	uint16_t max_lines;
	uint16_t max_samples_per_line;
	uint8_t components_n;
	struct jpeg_frame_component *components;
};

struct jpeg_DQT_table {
	uint8_t precision; // 4 bits, 0 for 8 bit, 1 for 16 bit
	uint8_t destination; // 4 bits, 0|1|2|3
	uint16_t elements[64]; // 64 elements. Each element can either be 8 or 16 bits
};

struct jpeg_DQT_segment {
	uint8_t tables_n;
	struct jpeg_DQT_table tables[4]; // each segment can contain at most 6 tables
};

struct jpeg_DHT_table {
	uint8_t class;
	uint8_t id;
	uint8_t count[16]; // count[i] specifies the number of codes of length i
	uint8_t *values; // values[i*16+j] is the jth value of length i
};

struct jpeg_DHT_segment {
	uint8_t tables_n;
	struct jpeg_DHT_table tables[6]; // each segment can contain at most 6 tables
};

struct jpeg_segment {
	uint8_t marker;
	uint32_t data_len;
	union {
		struct jpeg_SOF_segment SOF;
		struct jpeg_DQT_segment DQT;
		struct jpeg_DHT_segment DHT;
	} data;
};

struct jpeg_datastream {};

bool try_jpeg(FILE *file);

#endif // !FINFO_JPEG_H
