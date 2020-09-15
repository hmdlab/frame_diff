#define SWAP(type,x,y) do {type t = x; x=y; y=t; } while (0)

/* ---------------------------- */
extern void
MedianSmoothing (
	unsigned char *img_arr[],
	int h_size,
	int v_size,
	int num_layers
) ;

extern unsigned char
uc_median(
	unsigned char *m,
	int n
);
/* ---------------------------- */
