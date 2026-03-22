#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "finfo_jpeg.h"
#include "finfo_utils.h"
#include "finfo_kitty.h"

int jpeg_parse_DQT(unsigned char *data, size_t size, struct jpeg_segment *segment) {
	if (size < 64+1) {
		return -1;
	}
	struct jpeg_DQT_segment *dqt = &segment->data.DQT;

	int consumed = 0;
	int tn = 0;
	dqt->tables_n = 0;
	while (consumed < size) {
		assert(consumed + 1 + 64 + 64*dqt->tables[tn].precision <= size);

		dqt->tables[tn].precision = data[consumed+0] >> 4;
		assert(dqt->tables[tn].precision == 0 || dqt->tables[tn].precision == 1);
		dqt->tables[tn].destination = data[consumed+0] & 0b00001111;
		assert(dqt->tables[tn].destination >= 0 && dqt->tables[tn].destination <= 3);

		for (int i=0; i<64; i++) {
			if (dqt->tables[tn].precision == 0) {
				dqt->tables[tn].elements[i] = data[consumed+i+1];
			} else {
				dqt->tables[tn].elements[i] = BE_bytes_to_int(data+consumed+1+(i*2), 2);
			}
		}

		consumed += 1 + 64 + 64*dqt->tables[tn].precision;
		tn++;
		dqt->tables_n++;
	}


	// temp
	for (int k=0; k<dqt->tables_n; k++) {
		printf("Quantization Table. Precision: %d, destination: %d\n", dqt->tables[k].precision, dqt->tables[k].destination);
		printf("[");
		for (int i=0; i<8; i++) {
			for (int j=0; j<8; j++) {
				printf("%02d ", dqt->tables[k].elements[i*8+j]);
			}
			i == 7 ? printf("\b]\n\n") : printf("\n ");
		}
	}
	return 0;
}

int jpeg_parse_SOF(unsigned char *data, size_t size, struct jpeg_segment *segment) {
	if (size < 6) {
		return -1;
	}

	struct jpeg_SOF_segment *sof = &segment->data.SOF;
	sof->components_n = data[5];
	if (size < 6+3*sof->components_n) {
		return -1;
	}

	sof->precision = data[0];
	sof->max_lines = BE_bytes_to_int(data+1, 2);
	sof->max_samples_per_line = BE_bytes_to_int(data+3, 2);

	unsigned char *components_start = data+6;
	sof->components = malloc(sizeof(struct jpeg_frame_component)*sof->components_n);
	for (int i=0; i<sof->components_n; i++) {
		sof->components[i].id = components_start[i*3+0];
		sof->components[i].horizontal_sampling_factor = components_start[i*3+1] >> 4;
		sof->components[i].vertical_sampling_factor = components_start[i*3+1] & 0b00001111;
		sof->components[i].destination = components_start[i*3+2];
	}

	// temp
	printf("Start of Frame. Precision: %d, max_lines: %d, max_samples_per_line: %d\n", sof->precision, sof->max_lines, sof->max_samples_per_line);
	for (int i=0; i<sof->components_n; i++) {
		struct jpeg_frame_component component = sof->components[i];
		printf("\t Component %d. %dx%d. Destination: %d\n",
		 component.id,
		 component.horizontal_sampling_factor, component.vertical_sampling_factor, 
		 component.destination);
	}

	return 0;
}

int jpeg_parse_DHT(unsigned char *data, size_t size, struct jpeg_segment *segment) {
	struct jpeg_DHT_segment *dht = &segment->data.DHT;

	int consumed = 0;
	int tn = 0;
	dht->tables_n = 0;
	while (consumed < size) {
		dht->tables[tn].class = data[0] >> 4;
		dht->tables[tn].id = data[0] & 0b00001111;
		memcpy(dht->tables[tn].count, data+consumed+1, 16);

		int parameters_n = 0;
		for (int i=0; i<16; i++) parameters_n+=dht->tables[tn].count[i];
		assert(consumed+1+16+parameters_n <= size);

		dht->tables[tn].values = malloc(sizeof(uint8_t)*parameters_n);
		memcpy(dht->tables[tn].values, data+consumed+17, parameters_n);

		consumed += 1 + 16 + parameters_n;
		tn++;
		dht->tables_n++;
	}

	// temp
	for (int tn=0; tn<dht->tables_n; tn++) {
		printf("Huffman Table. Class: %d, id: %d\n", dht->tables[tn].class, dht->tables[tn].id);
		for (int i=0; i<16; i++) {
			printf("\tSize %d: [", i);
			for (int j=0; j<dht->tables[tn].count[i]; j++) {
				printf("%02d ", dht->tables[tn].values[j]);
				if (j == dht->tables[tn].count[i]-1) printf("\b");
			}
			printf("]\n");
		}
	}
	return 0;
}

int jpeg_parse_SOS(unsigned char *data, size_t size, struct jpeg_segment *segment) {
	struct jpeg_SOS_segment *sos = &segment->data.SOS;

	sos->components_n = data[0];

	sos->components = malloc(sizeof(struct jpeg_SOS_component)* sos->components_n);

	for (int i=0; i<sos->components_n; i++) {
		sos->components[i].component_selector = (data+1)[i*2];
		sos->components[i].DC_table_selector = (data+1)[i*2+1] >> 4;
		sos->components[i].AC_table_selector = (data+1)[i*2+1] & 0b00001111;
	}

	// TODO: get other fields info

	// temp
	printf("Start of Scan\n");
	for (int i=0; i<sos->components_n; i++) {
		struct jpeg_SOS_component c = sos->components[i];
		printf("\tImage component. Scan component: %d, DC table: %d, AC table: %d\n", 
		 c.component_selector, 
		 c.DC_table_selector, c.AC_table_selector);
	}
	return 0;
}

bool try_jpeg(FILE *file) {
	printf("Trying jpeg...\n");
	unsigned char signature[2];
	fread(signature, 2, 1, file);
	if (!(signature[0] == JPEG_MARKER_START && signature[1] == JPEG_MARKER_SOI)) { return false; }

	unsigned char marker_bytes[2];
	unsigned char length[2];
	while (true) {
		// read maker
		if (fread(marker_bytes, 1, 2, file) < 2) { 
			printf("Error! Unexpected end of file.\n");
			return false; 
		}
		struct jpeg_segment segment = {};
		segment.marker = marker_bytes[1];

		if (marker_bytes[0] != JPEG_MARKER_START) { 
			printf("Error! Found byte: %02X when marker start was expected.\n", marker_bytes[0]);
			return false; 
		}

		// read len
		fread(length, 2, 1, file);
		// len is data_len + marker_size
		segment.data_len = BE_bytes_to_int(length, 2) - 2;

		printf("Found marker: %02X %02X, len: %d\n", marker_bytes[0], marker_bytes[1], segment.data_len);
		if (marker_bytes[1] == JPEG_MARKER_EOI) {
			printf("End of image.\n");
			break; 
		}

		unsigned char *segment_data = malloc(segment.data_len);
		fread(segment_data, segment.data_len, 1, file); 

		switch (segment.marker) {
			case JPEG_MARKER_SOS:
				jpeg_parse_SOS(segment_data, segment.data_len, &segment);
				break;
			case JPEG_MARKER_DHT:
				jpeg_parse_DHT(segment_data, segment.data_len, &segment);
				break;
			case JPEG_MARKER_DQT:
				jpeg_parse_DQT(segment_data, segment.data_len, &segment);
				break;
			case JPEG_MARKER_SOF0: // TODO can be easily extensible to other SOF, the parse function is the same
				jpeg_parse_SOF(segment_data, segment.data_len, &segment);
				break;
			default:
				printf("!!!Handling not implemented for this marker.\n");
				break;
		}

		// if last one was Start of Scan, seek until you find an FF. If FF is followed by 00 or is restart marker keep seeking
		if (marker_bytes[1] == JPEG_MARKER_SOS){
			unsigned char c;
			// fread is buffered so this is fine
			while (true) {
				if (fread(&c, 1, 1, file) == 0) { return false; }
				if (c != JPEG_MARKER_START) { continue; }

				// 0xFF can be followed by other 0xFF before the type byte
				while (true) { 
					if (fread(&c, 1, 1, file) == 0) { return false; } 
					if (c != JPEG_MARKER_START) { break; }
				}

				if (c == 0x0 || (c >= JPEG_MARKER_RESTART_MIN && c <= JPEG_MARKER_RESTART_MAX)) { 
					continue; 
				}

				fseek(file, -2, SEEK_CUR);
				break;
			}
		}
	}

	// Reset position to start of file for printing it
	fseek(file, 0, SEEK_SET);
	kitty_print_file(file);

	return true;
}
