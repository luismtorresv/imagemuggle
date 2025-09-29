#ifndef UTILS_CONC_H
#define UTILS_CONC_H
#include <pthread.h>

typedef struct {
  unsigned char ***src; // read
  unsigned char ***dst; // write
  int width, height, channels;
  int y0, y1; // range [y0, y1)

  // Convolution
  const float *kernel;
  int k;
  float factor;
  float bias;
  // Rotation
  float cx, cy, ang_rad;
  // Resize
  float scale_x, scale_y;
} WorkArgs;

// Creates 3D matrix [height][width][channels] contiguous in memory
unsigned char ***create3DMatrix(int height, int width, int channels);

// Launch N threads executing 'worker' with row division
int launch_threads_by_rows(void *(*worker)(void *), WorkArgs base,
                           int num_threads);

// I/O utilities (optional stb)
int loadPNG(const char *path, unsigned char ****out_px, int *w, int *h,
            int *ch);
int savePNG(const char *path, unsigned char ***px, int w, int h, int ch);

#endif
