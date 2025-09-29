#include "resize.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
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

int resize_concurrent(unsigned char ***src, int w, int h, int channels,
                      unsigned char ***dst, int nw, int nh, int num_threads) {
  WorkArgs base = {
      .src = src, .dst = dst, .width = w, .height = h, .channels = channels};

  // Create array of ResizeArgs for each thread
  ResizeArgs *thread_args = malloc(num_threads * sizeof(ResizeArgs));
  pthread_t *threads = malloc(num_threads * sizeof(pthread_t));

  // Check memory allocation
  if (!thread_args || !threads) {
    free(thread_args);
    free(threads);
    return -1;
  }

  int rows_per_thread = nh / num_threads;
  int remainder = nh % num_threads;

  for (int i = 0; i < num_threads; i++) {
    thread_args[i].a = base;
    thread_args[i].nw = nw;
    thread_args[i].nh = nh;

    // Calculate row range for this thread
    thread_args[i].a.y0 = i * rows_per_thread + (i < remainder ? i : remainder);
    thread_args[i].a.y1 =
        thread_args[i].a.y0 + rows_per_thread + (i < remainder ? 1 : 0);

    // Check pthread_create
    if (pthread_create(&threads[i], NULL, worker_resize, &thread_args[i]) !=
        0) {
      // Join already created threads
      for (int j = 0; j < i; j++) {
        pthread_join(threads[j], NULL);
      }
      free(thread_args);
      free(threads);
      return -1;
    }
  }

  // Wait for all threads
  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  free(thread_args);
  free(threads);
  return 0;
}
