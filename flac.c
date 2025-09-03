#include <stdio.h>
#include <stdbool.h>
#include "flac.h"

bool try_flac(FILE *file) {
	char header[4];
	fread(header, 4, 1, file);

	return false;
}
