#include <stdio.h>
#include <stdbool.h>
#include "flac.h"

int main(int argc, char *argv[]) {
	printf("Hello bella pe' te\n");

	for (int i = 0; i < argc; printf("- %s\n", argv[i++])) {}

	FILE *file = fopen(argv[1], "rb");

	try_flac(file);

	return 0;
}
