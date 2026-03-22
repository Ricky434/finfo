#include "finfo_utils.h"
#include <sys/ioctl.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "libs/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "libs/stb_image_resize2.h"

// ===== Kitty image protocol printers =====

#define KITTY_ESCAPE_START "\033_G"
#define KITTY_ESCAPE_END "\033\\"
#define KITTY_CHUNK_SIZE 4096

void kitty_print_file(FILE *file) {
	int width, height, channels_n;
	unsigned char *data = stbi_load_from_file(file, &width, &height, &channels_n, 0);
	if (!data) {
		printf("%s", stbi_failure_reason());
		return;
	}
	printf("channels: %d, width: %d, height:%d\n", channels_n, width, height);

	struct winsize sz;
	ioctl(0, TIOCGWINSZ, &sz);

	float term_col_height_px = (float)sz.ws_ypixel / sz.ws_row;

	int clamped_width = (width < sz.ws_xpixel ? width : sz.ws_xpixel);
	int clamped_height = clamped_width * ((float)height/width);
	// Check if clamped sizes still allow for picture too high (also account for new prompt size)
	if (clamped_height > (sz.ws_ypixel - 3*term_col_height_px)) {
		clamped_height = sz.ws_ypixel - 3*term_col_height_px;
		clamped_width = clamped_height * ((float)width/height);
	}

	unsigned char *resized = stbir_resize_uint8_linear(data, width, height, 0, 
				NULL, clamped_width, clamped_height, 0, channels_n == 3 ? STBIR_RGB : STBIR_RGBA);
	stbi_image_free(data);

	char control_codes[50];
	snprintf(control_codes, sizeof(control_codes), ",a=T,f=%d,s=%d,v=%d",
			channels_n == 3 ? 24 : 32, clamped_width, clamped_height);

	// Send raw RGB pixel data (already decoded by stbi) in chunks
    unsigned char *ptr = resized;
    size_t total = (size_t)clamped_width * clamped_height * channels_n;
    size_t remaining = total;

    while (remaining > 0) {
        size_t chunk = remaining < KITTY_CHUNK_SIZE ? remaining : KITTY_CHUNK_SIZE;
        remaining -= chunk;
        int last = remaining == 0;

        size_t encoded_len = chunk;
        char *encoded = base64_encode(ptr, &encoded_len);
        ptr += chunk;

        printf("%sm=%d%s;%.*s%s", KITTY_ESCAPE_START, !last, control_codes,
               (int)encoded_len, encoded, KITTY_ESCAPE_END);

        free(encoded);

        // Control codes only in first chunk
        *control_codes = '\0';
    }

	free(resized);

	putchar('\n');
}

void kitty_print(unsigned char *data, size_t data_len) {
	FILE *file = fmemopen(data, data_len, "rb");
	kitty_print_file(file);
	fclose(file);
}

