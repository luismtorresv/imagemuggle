#include <stdio.h>
#include <math.h>
#include "conv.h"

static inline unsigned char clampi(int v){
    return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
}
static inline void clamp_xy(int* xx, int* yy, int w, int h){
    if (*xx < 0) *xx = 0; if (*xx >= w) *xx = w - 1;
    if (*yy < 0) *yy = 0; if (*yy >= h) *yy = h - 1;
}

static void* worker_conv(void* p){
    WorkArgs* a = (WorkArgs*)p;
    int r = a->k / 2;
    for (int y = a->y0; y < a->y1; y++){
        for (int x = 0; x < a->ancho; x++){
            for (int c = 0; c < a->canales; c++){
                float acc = 0.0f;
                for (int ky = -r; ky <= r; ky++){
                    for (int kx = -r; kx <= r; kx++){
                        int yy = y + ky, xx = x + kx;
                        clamp_xy(&xx, &yy, a->ancho, a->alto);
                        int ki = (ky + r)*a->k + (kx + r);
                        acc += a->src[yy][xx][c] * a->kernel[ki];
                    }
                }
                int val = (int)roundf(acc * a->factor + a->bias);
                a->dst[y][x][c] = clampi(val);
            }
        }
    }
    return NULL;
}

int conv_concurrente(unsigned char*** src,
                     unsigned char*** dst,
                     int ancho, int alto, int canales,
                     const float* kernel, int k,
                     float factor, float bias,
                     int num_hilos){
    WorkArgs base = {.src=src,.dst=dst,.ancho=ancho,.alto=alto,.canales=canales,
                     .kernel=kernel,.k=k,.factor=factor,.bias=bias};
    return lanzar_hilos_por_filas(worker_conv, base, num_hilos);
}
