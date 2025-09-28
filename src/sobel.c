#include <stdio.h>
#include <math.h>
#include "sobel.h"

static inline unsigned char clampi(int v){
    return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
}
static inline unsigned char to_gray(unsigned char* px, int channels){
    if (channels == 1) return px[0];
    return (unsigned char)(0.299f*px[0] + 0.587f*px[1] + 0.114f*px[2]);
}
static inline void clamp_xy(int* x, int* y, int width, int height){
    if (*x < 0) *x = 0; if (*x >= width) *x = width - 1;
    if (*y < 0) *y = 0; if (*y >= height) *y = height - 1;
}

static void* worker_sobel(void* p){
    WorkArgs* a = (WorkArgs*)p;
    int Gx[9] = {-1,0,1,-2,0,2,-1,0,1};
    int Gy[9] = {-1,-2,-1,0,0,0,1,2,1};
    for (int y = a->y0; y < a->y1; y++){
        for (int x = 0; x < a->width; x++){
            float sx = 0.f, sy = 0.f; int idx = 0;
            for (int ky=-1; ky<=1; ky++){
                for (int kx=-1; kx<=1; kx++, idx++){
                    int xx = x + kx, yy = y + ky;
                    clamp_xy(&xx, &yy, a->width, a->height);
                    unsigned char g = to_gray(a->src[yy][xx], a->channels);
                    sx += g * Gx[idx];
                    sy += g * Gy[idx];
                }
            }
            int mag = (int)roundf(sqrtf(sx*sx + sy*sy));
            unsigned char v = clampi(mag);
            if (a->channels == 1) a->dst[y][x][0] = v;
            else a->dst[y][x][0] = a->dst[y][x][1] = a->dst[y][x][2] = v;
        }
    }
    return NULL;
}

int sobel_concurrente(unsigned char*** src,
                      unsigned char*** dst,
                      int width, int height, int channels,
                      int num_threads){
    WorkArgs base = {.src=src,.dst=dst,.width=width,.height=height,.channels=channels};
    return launch_threads_by_rows(worker_sobel, base, num_threads);
}
