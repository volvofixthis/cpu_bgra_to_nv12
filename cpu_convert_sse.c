/*
 * CPU bgr0 -> nv12 frame convertation
 */

// System includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>

#include <xmmintrin.h> // sse
#include <emmintrin.h> // sse2
#include <pmmintrin.h> // sse3
#include <tmmintrin.h> // ssse3

#define SEC2NANO 1000000000.0f
#define BILLION 1000000000L

#define ReadBGRAInterleaved(ptr1, ptr2, ca, cb, r, g, b) \
    __m128i ca = _mm_loadu_si128((__m128i*) (ptr1)), cb = _mm_loadu_si128((__m128i*) (ptr2)); \
    __m128i r = _mm_or_si128(_mm_and_si128(_mm_srli_si128(ca, 2), v_byte1), _mm_and_si128(               cb    , v_byte3)); \
    __m128i g = _mm_or_si128(_mm_and_si128(_mm_srli_si128(ca, 1), v_byte1), _mm_and_si128(_mm_slli_si128(cb, 1), v_byte3)); \
    __m128i b = _mm_or_si128(_mm_and_si128(               ca    , v_byte1), _mm_and_si128(_mm_slli_si128(cb, 2), v_byte3));
#define Convert_RGB_Y(r, g, b, y) \
    __m128i y = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r, v_mat_yr), _mm_mullo_epi16(g, v_mat_yg)), _mm_add_epi16(_mm_mullo_epi16(b, v_mat_yb), v_offset_y));
#define Convert_RGB_U(r, g, b, u) \
    __m128i u = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r, v_mat_ur), _mm_mullo_epi16(g, v_mat_ug)), _mm_add_epi16(_mm_mullo_epi16(b, v_mat_ub_vr), v_offset_uv));
#define Convert_RGB_V(r, g, b, v) \
    __m128i v = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r, v_mat_ub_vr), _mm_mullo_epi16(g, v_mat_vg)), _mm_add_epi16(_mm_mullo_epi16(b, v_mat_vb), v_offset_uv));
#define WritePlaneInterleaved(ptr, y1, y2) \
    _mm_stream_si128((__m128i*) (ptr), _mm_or_si128(_mm_shuffle_epi8(y1, v_shuffle1), _mm_shuffle_epi8(y2, v_shuffle2)));
    
void Convert_BGRA_NV12_SSSE3(unsigned int w, unsigned int h, const uint8_t* in_data, int in_stride, uint8_t* const out_data[3], const int out_stride[3]) {
    assert(w % 2 == 0 && h % 2 == 0);
    assert((uintptr_t) out_data[0] % 16 == 0 && out_stride[0] % 16 == 0);
    assert((uintptr_t) out_data[1] % 16 == 0 && out_stride[1] % 16 == 0);

    __m128i v_byte1     = _mm_set1_epi32(0x000000ff);
    __m128i v_byte3     = _mm_set1_epi32(0x00ff0000);
    __m128i v_mat_yr    = _mm_set1_epi16(47);
    __m128i v_mat_yg    = _mm_set1_epi16(157);
    __m128i v_mat_yb    = _mm_set1_epi16(16);
    __m128i v_mat_ur    = _mm_set1_epi16(-26);
    __m128i v_mat_ug    = _mm_set1_epi16(-86);
    __m128i v_mat_ub_vr = _mm_set1_epi16(112);
    __m128i v_mat_vg    = _mm_set1_epi16(-102);
    __m128i v_mat_vb    = _mm_set1_epi16(-10);
    __m128i v_offset_y  = _mm_set1_epi16(128 + (16 << 8));
    __m128i v_offset_uv = _mm_set1_epi16(128 + (128 << 8));
    __m128i v_2         = _mm_set1_epi16(2);
    __m128i v_shuffle1  = _mm_setr_epi8(1, 5, 9, 13, 3, 7, 11, 15, 255, 255, 255, 255, 255, 255, 255, 255);
    __m128i v_shuffle2  = _mm_setr_epi8(255, 255, 255, 255, 255, 255, 255, 255, 1, 5, 9, 13, 3, 7, 11, 15);
    __m128i v_shuffle3  = _mm_setr_epi8(1, 5, 3, 7, 9, 13, 11, 15, 255, 255, 255, 255, 255, 255, 255, 255);

    const int offset_y = 128 + (16 << 8), offset_uv = (128 + (128 << 8)) << 2;

    for(unsigned int j = 0; j < h / 2; ++j) {
        const uint32_t *rgb1 = (const uint32_t*) (in_data + in_stride * (int) (j * 2));
        const uint32_t *rgb2 = (const uint32_t*) (in_data + in_stride * (int) (j * 2 + 1));
        uint8_t *yuv_y1 = out_data[0] + out_stride[0] * (int) (j * 2);
        uint8_t *yuv_y2 = out_data[0] + out_stride[0] * (int) (j * 2 + 1);
        uint8_t *yuv_uv = out_data[1] + out_stride[1] * (int) j;
        uint8_t yuv_u_arr[8];
        uint8_t yuv_v_arr[8];
        for(unsigned int i = 0; i < w / 16; ++i) {
            __m128i ra, ga, ba;
            {
                ReadBGRAInterleaved(rgb1    , rgb1 +  4, ca1, cb1, r1, g1, b1);
                ReadBGRAInterleaved(rgb1 + 8, rgb1 + 12, ca2, cb2, r2, g2, b2);
                rgb1 += 16;
                Convert_RGB_Y(r1, g1, b1, y1);
                Convert_RGB_Y(r2, g2, b2, y2);
                WritePlaneInterleaved(yuv_y1, y1, y2);
                yuv_y1 += 16;
                _mm_prefetch(rgb1 + 16, _MM_HINT_T0);
                ra = _mm_hadd_epi32(r1, r2);
                ga = _mm_hadd_epi32(g1, g2);
                ba = _mm_hadd_epi32(b1, b2);
            }
            {
                ReadBGRAInterleaved(rgb2    , rgb2 +  4, ca1, cb1, r1, g1, b1);
                ReadBGRAInterleaved(rgb2 + 8, rgb2 + 12, ca2, cb2, r2, g2, b2);
                rgb2 += 16;
                Convert_RGB_Y(r1, g1, b1, y1);
                Convert_RGB_Y(r2, g2, b2, y2);
                WritePlaneInterleaved(yuv_y2, y1, y2);
                yuv_y2 += 16;
                _mm_prefetch(rgb2 + 16, _MM_HINT_T0);
                ra = _mm_add_epi16(ra, _mm_hadd_epi32(r1, r2));
                ga = _mm_add_epi16(ga, _mm_hadd_epi32(g1, g2));
                ba = _mm_add_epi16(ba, _mm_hadd_epi32(b1, b2));
            }
            {
                ra = _mm_srli_epi16(_mm_add_epi16(ra, v_2), 2);
                ga = _mm_srli_epi16(_mm_add_epi16(ga, v_2), 2);
                ba = _mm_srli_epi16(_mm_add_epi16(ba, v_2), 2);
                __m128i u = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(ra, v_mat_ur), _mm_mullo_epi16(ga, v_mat_ug)), _mm_add_epi16(_mm_mullo_epi16(ba, v_mat_ub_vr), v_offset_uv));
                _mm_storel_epi64((__m128i*) yuv_u_arr, _mm_shuffle_epi8(u, v_shuffle3));
                __m128i v = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(ra, v_mat_ub_vr), _mm_mullo_epi16(ga, v_mat_vg)), _mm_add_epi16(_mm_mullo_epi16(ba, v_mat_vb), v_offset_uv));
                _mm_storel_epi64((__m128i*) yuv_v_arr, _mm_shuffle_epi8(v, v_shuffle3));
                for(unsigned int z = 0; z < 8; z++) {
                    *(yuv_uv) = yuv_u_arr[z];
                    *(yuv_uv+1) = yuv_v_arr[z];
                    yuv_uv += 2;
                }
            }
        }
        for(unsigned int i = 0; i < (w & 15) / 2; ++i) {
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

    _mm_sfence();

}

int main(int argc, char **argv)
{
    int height = 1080;
    int width = 1920;
    int pan_size = width*height*4;
    int pan_size_nv12 = width*height/2*3;
    int line,x, res;
    printf("CPU SSE bgra to nv12\n");
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
    
    bgr0 = (uint8_t *)malloc(pan_size);
    //nv12 = (unsigned char *)malloc(sizeof(char)*pan_size_nv12);
    //out_data[0] = (uint8_t*)malloc(width*height);
    //out_data[1] = (uint8_t*)malloc(width*height/2);
    res = posix_memalign((void **)&out_data[0],16,width*height);
    res = posix_memalign((void **)&out_data[1],16,width*height/2);
    
    
    ifp = fopen("test.bgr0", "rb");
    res = fread(bgr0,1,pan_size,ifp); 
    fclose(ifp);
    
    clock_gettime(CLOCK_MONOTONIC_RAW,&start);
    
    /*Do something*/
    Convert_BGRA_NV12_SSSE3(width,height,bgr0,width*4,out_data,out_stride);
    
    clock_gettime(CLOCK_MONOTONIC_RAW,&end);
    diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
    printf("Convertation took: %fs\n",diff/SEC2NANO);
    
    ifp = fopen("test.nv12", "w");
    fwrite(out_data[0],1,width*height,ifp);
    fwrite(out_data[1],1,width*height/2,ifp);
    fclose(ifp);
    
    
}




