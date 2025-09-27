#include <math.h>
#include "rotate.h"

static unsigned char bilinear(unsigned char*** src, int w, int h, int c, float xf, float yf){
    int x0 = (int)floorf(xf), y0 = (int)floorf(yf);
    int x1 = x0 + 1, y1 = y0 + 1;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 >= w) x1 = w-1; if (y1 >= h) y1 = h-1;
    float tx = xf - x0, ty = yf - y0;
    float v00 = src[y0][x0][c], v10 = src[y0][x1][c];
    float v01 = src[y1][x0][c], v11 = src[y1][x1][c];
    float v0 = v00*(1-tx) + v10*tx;
    float v1 = v01*(1-tx) + v11*tx;
    return (unsigned char)lrintf(v0*(1-ty) + v1*ty);
}

static void* worker_rotate(void* p){
    WorkArgs* a = (WorkArgs*)p;
    float cosA = cosf(a->ang_rad), sinA = sinf(a->ang_rad);
    float cx = a->cx, cy = a->cy;
    for (int y = a->y0; y < a->y1; y++){
        for (int x = 0; x < a->ancho; x++){
            float xd = x - cx, yd = y - cy;
            // mapeo inverso
            float xs =  cosA*xd + sinA*yd + cx;
            float ys = -sinA*xd + cosA*yd + cy;
            for (int c = 0; c < a->canales; c++){
                unsigned char val = 0;
                if (xs >= 0 && xs < a->ancho && ys >= 0 && ys < a->alto)
                    val = bilinear(a->src, a->ancho, a->alto, c, xs, ys);
                a->dst[y][x][c] = val;
            }
        }
    }
    return NULL;
}

int rotate_concurrente(unsigned char*** src, unsigned char*** dst,
                       int ancho, int alto, int canales,
                       float ang_deg, int num_hilos){
    WorkArgs base = {.src=src,.dst=dst,.ancho=ancho,.alto=alto,.canales=canales};
    base.cx = (ancho-1)/2.0f; base.cy = (alto-1)/2.0f;
    base.ang_rad = ang_deg * (float)M_PI / 180.0f;
    return lanzar_hilos_por_filas(worker_rotate, base, num_hilos);
}
