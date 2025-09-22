#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "finfo_flac.h"
#include "finfo_png.h"

int main(int argc, char *argv[]) {
	for (int i = 0; i < argc; printf("- %s\n", argv[i++])) {}

	if (argc != 2 || strcmp(argv[1], "-h") == 0 ||
		strcmp(argv[1], "--help") == 0) {
		printf("Usage: %s FILE\n", argv[0]);
		return 1;
	}

	FILE *file = fopen(argv[1], "rb");
	if (file == NULL) {
		printf("Unable to open file: %s (%s).\n", argv[1], strerror(errno));
		return 1;
	}

	enum { FILE_TYPES_N = 2 };
	bool (*try_type[FILE_TYPES_N])(FILE *) = {try_flac, try_png};

	for (int i = 0; i < FILE_TYPES_N; i++) {
		// Reset read position in file to make it ready for next try.
		fseek(file, 0, SEEK_SET); // TODO: check error

		if (try_type[i](file)) {
			fclose(file);
			return 0;
		}
	}

	fclose(file);
	return 1;
}
