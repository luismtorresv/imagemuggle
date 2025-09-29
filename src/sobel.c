#include "sobel.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * Converts a pixel to grayscale value using luminance weights.
 *
 * For single-channel images, returns the pixel value directly.
 * For multi-channel images, applies the standard RGB to grayscale conversion
 * formula using weighted average: 0.30*R + 0.59*G + 0.11*B
 *
 * @param px Pointer to pixel data array
 * @param channels Number of channels in the pixel data (1 for grayscale, 3+ for
 * color)
 * @return Grayscale value as unsigned char (0-255)
 */
static inline unsigned char to_gray(unsigned char *px, int channels) {
  if (channels == 1)
    return px[0];
  return (unsigned char)(0.30f * px[0] + 0.59f * px[1] + 0.11f * px[2]);
}

/**
 * Clamps x and y coordinates to valid image boundaries
 *
 * This function ensures that the given x and y coordinates stay within
 * the valid bounds of an image with the specified width and height.
 * Coordinates that fall outside the boundaries are adjusted to the
 * nearest valid edge value.
 *
 * @param x Pointer to the x coordinate to be clamped (modified in-place)
 * @param y Pointer to the y coordinate to be clamped (modified in-place)
 * @param width The width of the image (valid x range: 0 to width-1)
 * @param height The height of the image (valid y range: 0 to height-1)
 */
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

/**
 * Fills a 3x3 neighborhood memory array with grayscale values for Sobel edge
 * detection
 *
 * This function extracts a 3x3 pixel neighborhood around the specified
 * coordinates (x,y) from the input image buffer and converts each pixel to
 * grayscale. The resulting values are stored in a linear array for use in
 * convolution operations. Boundary pixels are handled by padding with zeros
 * when accessing pixels outside the image bounds.
 *
 * @param buffer 3D array representing the image [height][width][channels]
 * @param width Width of the image in pixels
 * @param height Height of the image in pixels
 * @param channels Number of color channels per pixel
 * @param x X-coordinate of the center pixel
 * @param y Y-coordinate of the center pixel
 * @param op_mem Output array of size 9 to store the 3x3 neighborhood grayscale
 * values Arranged as: [top-left, top-center, top-right, middle-left, center,
 * middle-right, bottom-left, bottom-center, bottom-right]
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

/**
 * Performs a 1D convolution operation between an input array and a kernel.
 *
 * This function computes the convolution by multiplying corresponding elements
 * of the input array X with the reversed kernel Y, then summing all products.
 * The kernel is accessed in reverse order (Y[c_size-i-1]) which is typical
 * for convolution operations.
 *
 * @param X Pointer to the input array of unsigned char values
 * @param Y Pointer to the convolution kernel array of int values
 * @param c_size Size of both arrays (must be the same for both X and Y)
 * @return The computed convolution result as an integer sum
 */
static int convolution(unsigned char *X, int *Y, int c_size) {
  int sum = 0;
  for (int i = 0; i < c_size; i++) {
    sum += X[i] * Y[c_size - i - 1];
  }
  return sum;
}

/**
 * Worker thread function that applies Sobel edge detection to a portion of an
 * image
 *
 * This function implements the Sobel edge detection algorithm using horizontal
 * and vertical convolution kernels to detect edges in an image. It processes a
 * specific range of rows as determined by the WorkArgs structure, making it
 * suitable for parallel processing.
 *
 * The Sobel operator uses two 3x3 kernels:
 * - Horizontal kernel: [-1, 0, 1, -2, 0, 2, -1, 0, 1] (detects vertical edges)
 * - Vertical kernel: [1, 2, 1, 0, 0, 0, -1, -2, -1] (detects horizontal edges)
 *
 * The edge magnitude is calculated as sqrt(gx² + gy²) where gx and gy are the
 * horizontal and vertical gradients respectively. The result is clamped to the
 * range [0, 255] for 8-bit output.
 *
 * @param p Pointer to WorkArgs structure containing:
 *          - src: Source image data
 *          - dst: Destination image data
 *          - width: Image width in pixels
 *          - height: Image height in pixels
 *          - channels: Number of color channels (1 for grayscale, 3+ for color)
 *          - y0: Starting row for this worker thread
 *          - y1: Ending row (exclusive) for this worker thread
 *
 * @return NULL (standard pthread worker function return)
 *
 * @note For multi-channel images, the same edge value is applied to all
 * channels
 * @note Requires makeOpMem() and convolution() helper functions to be available
 * @note Uses sqrt() from math.h and abs() from stdlib.h
 */
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

/**
 * Applies Sobel edge detection filter to an image using multiple threads
 *
 * This function performs concurrent Sobel edge detection on a source image by
 * dividing the work among multiple threads. Each thread processes a subset of
 * image rows to compute the Sobel gradient magnitude for edge detection.
 *
 * @param src Pointer to 3D array containing source image data
 * [height][width][channels]
 * @param dst Pointer to 3D array to store filtered image data
 * [height][width][channels]
 * @param width Width of the image in pixels
 * @param height Height of the image in pixels
 * @param channels Number of color channels in the image (e.g., 3 for RGB)
 * @param num_threads Number of worker threads to use for parallel processing
 *
 * @return Status code indicating success or failure of the operation
 *
 * @note The source and destination arrays must be pre-allocated with sufficient
 * memory
 * @note Thread-safe implementation divides image rows among worker threads
 */
int sobel_concurrent(unsigned char ***src, unsigned char ***dst, int width,
                     int height, int channels, int num_threads) {
  WorkArgs base = {.src = src,
                   .dst = dst,
                   .width = width,
                   .height = height,
                   .channels = channels};
  return launch_threads_by_rows(worker_sobel, base, num_threads);
}
