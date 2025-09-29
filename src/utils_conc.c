#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define USE_STB if stb headers are provided under -Ithird_party
#ifdef USE_STB
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#endif

#include "utils_conc.h"

unsigned char ***create3DMatrix(int height, int width, int channels) {
  // contiguous layout: [height * width * channels] + pointer tables
  unsigned char *data =
      (unsigned char *)calloc((size_t)height * width * channels, 1);
  if (!data)
    return NULL;
  unsigned char ***m =
      (unsigned char ***)malloc(sizeof(unsigned char **) * height);
  if (!m) {
    free(data);
    return NULL;
  }
  for (int y = 0; y < height; y++) {
    m[y] = (unsigned char **)malloc(sizeof(unsigned char *) * width);
    if (!m[y]) {
      for (int i = 0; i < y; i++)
        free(m[i]);
      free(m);
      free(data);
      return NULL;
    }
    for (int x = 0; x < width; x++) {
      m[y][x] = data + ((size_t)y * width + x) * channels;
    }
  }
  return m;
}

int launch_threads_by_rows(void *(*worker)(void *), WorkArgs base,
                           int num_threads) {
  if (num_threads < 1)
    num_threads = 1;
  pthread_t *tids = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
  WorkArgs *args = (WorkArgs *)malloc(sizeof(WorkArgs) * num_threads);
  if (!tids || !args) {
    perror("malloc");
    free(tids);
    free(args);
    return -1;
  }
  int rows = base.height;
  int per_thread = (int)ceil((double)rows / num_threads);
  for (int i = 0; i < num_threads; i++) {
    args[i] = base;
    args[i].y0 = i * per_thread;
    args[i].y1 = args[i].y0 + per_thread;
    if (args[i].y0 > rows)
      args[i].y0 = rows;
    if (args[i].y1 > rows)
      args[i].y1 = rows;
    if (pthread_create(&tids[i], NULL, worker, &args[i]) != 0) {
      perror("pthread_create");
      free(tids);
      free(args);
      return -1;
    }
  }
  for (int i = 0; i < num_threads; i++)
    pthread_join(tids[i], NULL);
  free(tids);
  free(args);
  return 0;
}

// ------- Optional: I/O with stb --------
int loadPNG(const char *path, unsigned char ****out_px, int *w, int *h,
            int *ch) {
#ifndef USE_STB
  fprintf(stderr,
          "[WARN] loadPNG requires stb (define USE_STB and include headers)\n");
  return -1;
#else
  int x, y, c;
  unsigned char *data = stbi_load(path, &x, &y, &c, 0);
  if (!data) {
    fprintf(stderr, "Error loading %s\n", path);
    return -1;
  }
  *w = x;
  *h = y;
  *ch = c;
  unsigned char ***m = create3DMatrix(y, x, c);
  if (!m) {
    stbi_image_free(data);
    return -1;
  }
  for (int yy = 0; yy < y; yy++) {
    for (int xx = 0; xx < x; xx++) {
      memcpy(m[yy][xx], data + (size_t)(yy * x + xx) * c, (size_t)c);
    }
  }
  stbi_image_free(data);
  *out_px = m;
  return 0;
#endif
}

int savePNG(const char *path, unsigned char ***px, int w, int h, int ch) {
#ifndef USE_STB
  fprintf(stderr,
          "[WARN] savePNG requires stb (define USE_STB and include headers)\n");
  return -1;
#else
  unsigned char *flat = (unsigned char *)malloc((size_t)w * h * ch);
  if (!flat)
    return -1;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      memcpy(flat + (size_t)(y * w + x) * ch, px[y][x], (size_t)ch);
    }
  }
  int ok = stbi_write_png(path, w, h, ch, flat, w * ch);
  free(flat);
  return ok ? 0 : -1;
#endif
}
