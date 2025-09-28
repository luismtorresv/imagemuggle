#include "resize.h"
#include <math.h>
#include <string.h>

typedef struct {
  WorkArgs a;
  int nw, nh;
} ResizeArgs;

static unsigned char bilinear(unsigned char ***src, int w, int h, int c,
                              float xs, float ys) {
  int x0 = (int)floorf(xs), y0 = (int)floorf(ys);
  int x1 = x0 + 1, y1 = y0 + 1;
  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 >= w)
    x1 = w - 1;
  if (y1 >= h)
    y1 = h - 1;
  float tx = xs - x0, ty = ys - y0;
  float v00 = src[y0][x0][c], v10 = src[y0][x1][c];
  float v01 = src[y1][x0][c], v11 = src[y1][x1][c];
  float v0 = v00 * (1 - tx) + v10 * tx;
  float v1 = v01 * (1 - tx) + v11 * tx;
  return (unsigned char)lrintf(v0 * (1 - ty) + v1 * ty);
}

static void *worker_resize(void *p) {
  ResizeArgs *R = (ResizeArgs *)p;
  WorkArgs *a = &R->a;
  float sx = (float)a->width / (float)R->nw;
  float sy = (float)a->height / (float)R->nh;
  for (int y = a->y0; y < a->y1 && y < R->nh; y++) {
    for (int x = 0; x < R->nw; x++) {
      float xs = (x + 0.5f) * sx - 0.5f;
      float ys = (y + 0.5f) * sy - 0.5f;
      for (int c = 0; c < a->channels; c++) {
        a->dst[y][x][c] = bilinear(a->src, a->width, a->height, c, xs, ys);
      }
    }
  }
  return NULL;
}

int resize_concurrente(unsigned char ***src, int w, int h, int channels,
                       unsigned char ***dst, int nw, int nh, int num_threads) {
  WorkArgs base = {
      .src = src, .dst = dst, .width = w, .height = h, .channels = channels};
  ResizeArgs R;
  R.a = base;
  R.nw = nw;
  R.nh = nh;
  R.a.height = nh; // row division in destination
  return launch_threads_by_rows((void *(*)(void *))worker_resize, R.a,
                                num_threads);
}
