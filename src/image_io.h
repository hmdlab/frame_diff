#include <stdio.h>
#include <stdlib.h>

int get_bmp_image_size(
	int *h_size,
		int *v_size,
		FILE *fp
);

int image_input_bmp(
	unsigned char *image[3],
	int h_size,
	int v_size,
	FILE *fp
);

int image_output_bmp(
	unsigned char *image[3],
	int h_size,
	int v_size,
	FILE *fp
);

int int_from_uchars(
	unsigned char value_a,
		unsigned char value_b,
		unsigned char value_c,
		unsigned char value_d
);

void uchars_from_int(
	unsigned char *ret_a,
		unsigned char *ret_b,
		unsigned char *ret_c,
		unsigned char *ret_d,
		int value
);
