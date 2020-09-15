#include "median_filter.h"

/* ---------------------------- */
void MedianSmoothing (
    unsigned char *img_arr[], 
    int h_size,
    int v_size,
    int num_layers
) 
{
    unsigned char	p[9];
    int	t,i,j,pos;
    
    for(t=0;t<num_layers;t++) {
        for(j=1;j<(v_size-1);j++) {
            for(i=1;i<(h_size-1);i++) {
                pos=j*h_size+i;
                p[0]=img_arr[t][pos-h_size-1];
                p[1]=img_arr[t][pos-h_size];
                p[2]=img_arr[t][pos-h_size+1];
                p[3]=img_arr[t][pos-1];
                p[4]=img_arr[t][pos];
                p[5]=img_arr[t][pos+1];
                p[6]=img_arr[t][pos+h_size-1];
                p[7]=img_arr[t][pos+h_size];
                p[8]=img_arr[t][pos+h_size+1];
                img_arr[t][pos]=uc_median(p,9);
            } /* for (i) */
        } /* for (j) */
    } /* for (t) */
}
/* ---------------------------- */


/* ---------------------------- */
unsigned char uc_median(
    unsigned char *m,
    int n
)
{
    int i,j,mid;
    unsigned char median;

    /* bubble sort */
    for (i=0;i<n-1;i++) {
        for (j=n-1;j>i;j--) {
            if (m[j-1] > m[j])
                SWAP(unsigned char, m[j-1], m[j]);
        }
    }
    mid=n/2;
    median = m[mid];

    return(median);
}
/* ---------------------------- */

