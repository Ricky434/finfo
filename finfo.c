#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "flac.h"

int main(int argc, char *argv[]) {
	for (int i = 0; i < argc; printf("- %s\n", argv[i++])) {}

	if (argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		printf("Usage: %s FILE\n", argv[0]);
		return 1;
	}

	FILE *file = fopen(argv[1], "rb");
	if (file == NULL) {
		int err = errno;
		printf("Unable to open file: %s. Error: %d\n", argv[1], err);
		return 1;
	}

	try_flac(file);

	return 0;
}
