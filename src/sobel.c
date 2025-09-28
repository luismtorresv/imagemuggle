#include "sobel.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline unsigned char clampi(int v) {
  return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
}
static inline unsigned char to_gray(unsigned char *px, int channels) {
  if (channels == 1)
    return px[0];
  return (unsigned char)(0.30f * px[0] + 0.59f * px[1] + 0.11f * px[2]);
}
static inline void clamp_xy(int *x, int *y, int width, int height) {
  if (*x < 0)
    *x = 0;
  if (*x >= width)
    *x = width - 1;
  if (*y < 0)
    *y = 0;
  if (*y >= height)
    *y = height - 1;
}

/*
 * Make the operation memory for iterative convolution
 * Adapted from the reference implementation to work with 3D array structure
 */
static void makeOpMem(unsigned char ***buffer, int width, int height,
                      int channels, int x, int y, unsigned char *op_mem) {
  int bottom = y - 1 < 0;
  int top = y + 1 >= height;
  int left = x - 1 < 0;
  int right = x + 1 >= width;

  // Get grayscale values for the 3x3 neighborhood
  op_mem[0] = (!bottom && !left) ? to_gray(buffer[y - 1][x - 1], channels) : 0;
  op_mem[1] = !bottom ? to_gray(buffer[y - 1][x], channels) : 0;
  op_mem[2] = (!bottom && !right) ? to_gray(buffer[y - 1][x + 1], channels) : 0;

  op_mem[3] = !left ? to_gray(buffer[y][x - 1], channels) : 0;
  op_mem[4] = to_gray(buffer[y][x], channels);
  op_mem[5] = !right ? to_gray(buffer[y][x + 1], channels) : 0;

  op_mem[6] = (!top && !left) ? to_gray(buffer[y + 1][x - 1], channels) : 0;
  op_mem[7] = !top ? to_gray(buffer[y + 1][x], channels) : 0;
  op_mem[8] = (!top && !right) ? to_gray(buffer[y + 1][x + 1], channels) : 0;
}

/*
 * Performs convolution between first two arguments
 */
static int convolution(unsigned char *X, int *Y, int c_size) {
  int sum = 0;
  for (int i = 0; i < c_size; i++) {
    sum += X[i] * Y[c_size - i - 1];
  }
  return sum;
}

static void *worker_sobel(void *p) {
  WorkArgs *a = (WorkArgs *)p;

  // Sobel kernels from reference implementation
  int sobel_h[] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
  int sobel_v[] = {1, 2, 1, 0, 0, 0, -1, -2, -1};

  // Temporary memory for each pixel operation
  unsigned char op_mem[9];

  for (int y = a->y0; y < a->y1; y++) {
    for (int x = 0; x < a->width; x++) {
      // Make operation memory for current pixel
      makeOpMem(a->src, a->width, a->height, a->channels, x, y, op_mem);

      // Calculate horizontal and vertical gradients
      int gx = abs(convolution(op_mem, sobel_h, 9));
      int gy = abs(convolution(op_mem, sobel_v, 9));

      // Calculate contour (magnitude) - using sqrt like in reference
      int magnitude = (int)sqrt(gx * gx + gy * gy);

      // Clamp to 0-255 range
      if (magnitude > 255)
        magnitude = 255;

      unsigned char edge_val = (unsigned char)magnitude;

      // Set output pixel
      if (a->channels == 1) {
        a->dst[y][x][0] = edge_val;
      } else {
        a->dst[y][x][0] = edge_val;
        a->dst[y][x][1] = edge_val;
        a->dst[y][x][2] = edge_val;
      }
    }
  }

  return NULL;
}

int sobel_concurrente(unsigned char ***src, unsigned char ***dst, int width,
                      int height, int channels, int num_threads) {
  WorkArgs base = {.src = src,
                   .dst = dst,
                   .width = width,
                   .height = height,
                   .channels = channels};
  return launch_threads_by_rows(worker_sobel, base, num_threads);
}
