/*
 * CPU bgr0 -> nv12 frame convertation
 */

// System includes
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>

#define SEC2NANO 1000000000.0f
#define BILLION 1000000000L

void Convert_BGRA_NV12_Fallback(unsigned int w, unsigned int h, const uint8_t* in_data, int in_stride, uint8_t* const out_data[3], const int out_stride[3]) {
	assert(w % 2 == 0 && h % 2 == 0);
	const int offset_y = 128 + (16 << 8), offset_uv = (128 + (128 << 8)) << 2;
	for(unsigned int j = 0; j < h / 2; ++j) {
		const uint32_t *rgb1 = (const uint32_t*) (in_data + in_stride * (int) j * 2);
		const uint32_t *rgb2 = (const uint32_t*) (in_data + in_stride * ((int) j * 2 + 1));
		uint8_t *yuv_y1 = out_data[0] + out_stride[0] * (int) j * 2;
		uint8_t *yuv_y2 = out_data[0] + out_stride[0] * ((int) j * 2 + 1);
		uint8_t *yuv_uv = out_data[1] + out_stride[1] * (int) j;
		for(unsigned int i = 0; i < w / 2; ++i) {
			uint32_t c1 = rgb1[0], c2 = rgb1[1], c3 = rgb2[0], c4 = rgb2[1];
			rgb1 += 2; rgb2 += 2;
			int r1 = (int) ((c1 >> 16) & 0xff), r2 = (int) ((c2 >> 16) & 0xff), r3 = (int) ((c3 >> 16) & 0xff), r4 = (int) ((c4 >> 16) & 0xff);
			int g1 = (int) ((c1 >>  8) & 0xff), g2 = (int) ((c2 >>  8) & 0xff), g3 = (int) ((c3 >>  8) & 0xff), g4 = (int) ((c4 >>  8) & 0xff);
			int b1 = (int) ((c1      ) & 0xff), b2 = (int) ((c2      ) & 0xff), b3 = (int) ((c3      ) & 0xff), b4 = (int) ((c4      ) & 0xff);
			yuv_y1[0] = (47 * r1 + 157 * g1 + 16 * b1 + offset_y) >> 8;
			yuv_y1[1] = (47 * r2 + 157 * g2 + 16 * b2 + offset_y) >> 8;
			yuv_y2[0] = (47 * r3 + 157 * g3 + 16 * b3 + offset_y) >> 8;
			yuv_y2[1] = (47 * r4 + 157 * g4 + 16 * b4 + offset_y) >> 8;
			yuv_y1 += 2; yuv_y2 += 2;
			int sr = r1 + r2 + r3 + r4;
			int sg = g1 + g2 + g3 + g4;
			int sb = b1 + b2 + b3 + b4;
			*yuv_uv = (-26 * sr +  -86 * sg + 112 * sb + offset_uv) >> 10;
			++yuv_uv;
			*yuv_uv = (112 * sr + -102 * sg + -10 * sb + offset_uv) >> 10;
			++yuv_uv;
		}
	}
}

int main(int argc, char **argv)
{
	int height = 1080;
	int width = 1920;
	int pan_size = width*height*4;
	int pan_size_nv12 = width*height/2*3;
	int line,x, res;
    printf("CPU bgra to nv12\n");
    FILE *ifp;
	uint8_t *bgr0;
	unsigned char *nv12;
	uint8_t* out_data[3];
	int out_stride[3];
	struct timespec start;
    struct timespec end;
    uint64_t diff;
	out_stride[0] = width;
	out_stride[1] = width;
	//out_stride[2] = width;
	
	bgr0 = (unsigned char *)malloc(sizeof(char)*pan_size);
	//nv12 = (unsigned char *)malloc(sizeof(char)*pan_size_nv12);
	res = posix_memalign((void **)&out_data[0],16,width*height);
    res = posix_memalign((void **)&out_data[1],16,width*height/2);
	
	ifp = fopen("test.bgr0", "rb");
	res = fread(bgr0,1,pan_size,ifp); 
	fclose(ifp);
	
	clock_gettime(CLOCK_MONOTONIC_RAW,&start);
	
	/*Do something*/
	Convert_BGRA_NV12_Fallback(width,height,bgr0,width*4,out_data,out_stride);

	clock_gettime(CLOCK_MONOTONIC_RAW,&end);
	diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
	printf("Convertation took: %fs\n",diff/SEC2NANO);
	
	ifp = fopen("test.nv12", "w");
	fwrite(out_data[0],1,width*height,ifp);
	fwrite(out_data[1],1,width*height/2,ifp);
	fclose(ifp);
	
    
}




