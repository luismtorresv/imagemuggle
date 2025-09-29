#include "conv.h"
#include <math.h>
#include <stdio.h>

/**
 * Clamps an integer value to the valid range for an unsigned char (0-255).
 *
 * This function ensures that the input value is within the valid range for
 * an 8-bit unsigned integer. Values below 0 are clamped to 0, and values
 * above 255 are clamped to 255.
 *
 * @param v The integer value to clamp
 * @return The clamped value as an unsigned char in the range [0, 255]
 */
static inline unsigned char clampi(int v) {
  return (unsigned char)(v < 0 ? 0 : (v > 255 ? 255 : v));
}

/**
 * Clamps x and y coordinates to stay within image bounds
 *
 * Ensures that the given x and y coordinates are within the valid range
 * for an image of specified width and height. If coordinates are outside
 * the bounds, they are adjusted to the nearest valid boundary.
 *
 * @param xx Pointer to x coordinate to be clamped (modified in-place)
 * @param yy Pointer to y coordinate to be clamped (modified in-place)
 * @param w Width of the image (valid x range: 0 to w-1)
 * @param h Height of the image (valid y range: 0 to h-1)
 */
static inline void clamp_xy(int *xx, int *yy, int w, int h) {
  if (*xx < 0)
    *xx = 0;
  if (*xx >= w)
    *xx = w - 1;
  if (*yy < 0)
    *yy = 0;
  if (*yy >= h)
    *yy = h - 1;
}

/**
 * Worker thread function for performing 2D convolution on image data.
 *
 * This function applies a convolution kernel to a portion of an image,
 * processing pixels from row y0 to y1-1. The convolution is performed
 * on all channels of the image with proper boundary handling via clamping.
 *
 * @param p Pointer to WorkArgs structure containing:
 *          - src: Source image data (3D array: [height][width][channels])
 *          - dst: Destination image data (3D array: [height][width][channels])
 *          - kernel: Convolution kernel (1D array of size k*k)
 *          - width: Image width in pixels
 *          - height: Image height in pixels
 *          - channels: Number of color channels
 *          - k: Kernel size (assumed to be odd, e.g., 3, 5, 7)
 *          - y0: Starting row index (inclusive)
 *          - y1: Ending row index (exclusive)
 *          - factor: Scaling factor applied to convolution result
 *          - bias: Bias value added after scaling
 *
 * @return NULL (standard pthread worker return value)
 *
 * @note The kernel is applied with radius r = k/2, centered on each pixel.
 *       Out-of-bounds coordinates are handled by the clamp_xy function.
 *       Final pixel values are rounded and clamped to valid range.
 */
static void *worker_conv(void *p) {
  WorkArgs *a = (WorkArgs *)p;
  int r = a->k / 2;
  for (int y = a->y0; y < a->y1; y++) {
    for (int x = 0; x < a->width; x++) {
      for (int c = 0; c < a->channels; c++) {
        float acc = 0.0f;
        for (int ky = -r; ky <= r; ky++) {
          for (int kx = -r; kx <= r; kx++) {
            int yy = y + ky, xx = x + kx;
            clamp_xy(&xx, &yy, a->width, a->height);
            int ki = (ky + r) * a->k + (kx + r);
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

/**
 * Performs concurrent convolution operation on a 3D image array using multiple
 * threads.
 *
 * This function applies a convolution kernel to the source image and stores the
 * result in the destination array. The operation is parallelized across
 * multiple threads by distributing work by rows to improve performance on
 * multi-core systems.
 *
 * @param src        Source 3D image array (height x width x channels)
 * @param dst        Destination 3D image array to store convolution result
 * @param width      Width of the image in pixels
 * @param height     Height of the image in pixels
 * @param channels   Number of color channels in the image
 * @param kernel     Convolution kernel matrix (k x k)
 * @param k          Size of the convolution kernel (kernel is k x k)
 * @param factor     Scaling factor applied to convolution result
 * @param bias       Bias value added to convolution result after scaling
 * @param num_threads Number of worker threads to use for parallel processing
 *
 * @return           Status code indicating success or failure of the operation
 */
int conv_concurrent(unsigned char ***src, unsigned char ***dst, int width,
                    int height, int channels, const float *kernel, int k,
                    float factor, float bias, int num_threads) {
  WorkArgs base = {.src = src,
                   .dst = dst,
                   .width = width,
                   .height = height,
                   .channels = channels,
                   .kernel = kernel,
                   .k = k,
                   .factor = factor,
                   .bias = bias};
  return launch_threads_by_rows(worker_conv, base, num_threads);
}
