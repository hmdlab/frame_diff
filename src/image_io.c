#include <string.h>
#include "image_io.h"

int get_bmp_image_size(
	int *h_size,
	int *v_size,
	FILE *fp
)
{
unsigned char
	hdr[54];
	
	if(54 != fread(hdr,sizeof(unsigned char),54,fp)){
		fprintf(stderr,"bmp_header read error\n");
    	exit(0);
  }
	
  *h_size=int_from_uchars(hdr[21],hdr[20],hdr[19],hdr[18]);
  *v_size=int_from_uchars(hdr[25],hdr[24],hdr[23],hdr[22]);
	
  fseek(fp,0,0);
	
  return (0);
}


int image_input_bmp(
	unsigned char *image[3],
	int h_size,
	int v_size,
	FILE *fp
)
{
  unsigned char
    hdr[54];
	
  int
    pad,p,
    pixeldepth,
    colormap_num,
    image_size,
    is,
    i,j,col; 
	
  image_size=h_size*v_size;
  
	
	if(54 != fread(hdr,sizeof(unsigned char),54,fp)){
		fprintf(stderr,"bmp_header read error\n");
		exit(0);
	}
	
	pixeldepth  =int_from_uchars(      0,      0,hdr[29],hdr[28]);
	colormap_num=int_from_uchars(hdr[49],hdr[48],hdr[47],hdr[46]);
	
	if(pixeldepth==24){
		
		/* read image data */
		pad=(3*h_size)%4;
		is=(v_size-1)*h_size;
		for(j=v_size-1;j>=0;j--,is-=h_size){
			for(i=0;i<h_size;i++){
				for(col=2;col>=0;col--) image[col][is+i]=getc(fp);
			}
			if(pad>0){
				for(p=0;p<4-pad;p++) (void)getc(fp);
			}
		}
		
	}else{
		fprintf(stderr,"BMP depth=%d is not supported in image_input_bmp()\n",pixeldepth);
		exit(0);
	}
	
  return (0);
}

int image_output_bmp(
	unsigned char *image[3],
	int h_size,
	int v_size,
	FILE *fp
)
{
  unsigned char
    void_a,void_b,
    hdr[54];
	
  int
    pad,p,
    header_size,image_size,bitcount,
    sizey,
    is,
    i,j;
	
  sizey=h_size*v_size;
	
	header_size=54;
	bitcount=24;
	
	image_size=3*h_size*v_size;
	
		memset(hdr,0,54);
		
    /* BMP File Header */
    hdr[0]=0x42;
    hdr[1]=0x4d;
    uchars_from_int(&hdr[ 5],&hdr[ 4],&hdr[ 3],&hdr[ 2],header_size+image_size);
    uchars_from_int(&hdr[13],&hdr[12],&hdr[11],&hdr[10],header_size);
    
    /* BMP Info Header */
    uchars_from_int(&hdr[17],&hdr[16],&hdr[15],&hdr[14],40);
    uchars_from_int(&hdr[21],&hdr[20],&hdr[19],&hdr[18],h_size);
    uchars_from_int(&hdr[25],&hdr[24],&hdr[23],&hdr[22],v_size);
    uchars_from_int( &void_a, &void_b,&hdr[27],&hdr[26],1);
    uchars_from_int( &void_a, &void_b,&hdr[29],&hdr[28],bitcount);
    
    /* output header */
    fwrite(hdr,sizeof(unsigned char),54,fp);
		
		/* image data */
		pad=(3*h_size)%4;
		is=(v_size-1)*h_size;
		for(j=v_size-1;j>=0;j--,is-=h_size){
			for(i=0;i<h_size;i++){
				putc(image[2][is+i],fp);
				putc(image[1][is+i],fp);
				putc(image[0][is+i],fp);
			}
			
			if(pad>0){
				for(p=0;p<4-pad;p++) putc(0,fp);
			}
		}
		return (0);
}

int int_from_uchars(
	unsigned char value_a,
	unsigned char value_b,
	unsigned char value_c,
	unsigned char value_d
)
{
  int
    ret;
	
  ret=(((int)value_a)<<24)+(((int)value_b)<<16)+(((int)value_c)<<8)+((int)value_d);
	
  return ret;
}

void uchars_from_int(
	unsigned char *ret_a,
	unsigned char *ret_b,
	unsigned char *ret_c,
	unsigned char *ret_d,
	int value
)
{
  *ret_a=(unsigned char)( (value>>24) & 0x000000ff );
  *ret_b=(unsigned char)( (value>>16) & 0x000000ff );
  *ret_c=(unsigned char)( (value>> 8) & 0x000000ff );
  *ret_d=(unsigned char)( (value    ) & 0x000000ff );
	
  return;
}
