#include "rotate.h"
#include <math.h>

/**
 * Performs bilinear interpolation to sample a pixel value from a 3D image
 * array.
 *
 * This function calculates an interpolated pixel value at floating-point
 * coordinates by sampling the four nearest integer pixel coordinates and
 * computing a weighted average based on the fractional parts of the input
 * coordinates.
 *
 * The interpolation works in two steps:
 * 1. Linear interpolation along the x-axis between (x0,y0)-(x1,y0) and
 * (x0,y1)-(x1,y1)
 * 2. Linear interpolation along the y-axis between the results from step 1
 *
 * Boundary conditions are handled by clamping coordinates to valid image
 * bounds.
 *
 * @param src    3D array representing the source image
 * [height][width][channels]
 * @param w      Width of the source image in pixels
 * @param h      Height of the source image in pixels
 * @param c      Channel index to sample from
 * @param xf     Floating-point x-coordinate to sample at
 * @param yf     Floating-point y-coordinate to sample at
 *
 * @return       Interpolated pixel value as an unsigned char, rounded to
 * nearest integer
 */
static unsigned char bilinear(unsigned char ***src, int w, int h, int c,
                              float xf, float yf) {
  int x0 = (int)floorf(xf), y0 = (int)floorf(yf);
  int x1 = x0 + 1, y1 = y0 + 1;
  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 >= w)
    x1 = w - 1;
  if (y1 >= h)
    y1 = h - 1;
  float tx = xf - x0, ty = yf - y0;
  float v00 = src[y0][x0][c], v10 = src[y0][x1][c];
  float v01 = src[y1][x0][c], v11 = src[y1][x1][c];
  float v0 = v00 * (1 - tx) + v10 * tx;
  float v1 = v01 * (1 - tx) + v11 * tx;
  return (unsigned char)lrintf(v0 * (1 - ty) + v1 * ty);
}

/**
 * Worker thread function for image rotation using bilinear interpolation.
 *
 * This function rotates a portion of an image by applying a rotation matrix
 * to map destination pixels back to source coordinates (inverse mapping).
 * Uses bilinear interpolation for smooth pixel value computation.
 *
 * @param p Pointer to WorkArgs structure containing:
 *          - ang_rad: Rotation angle in radians
 *          - cx, cy: Center of rotation coordinates
 *          - y0, y1: Y-coordinate range for this worker thread
 *          - width, height: Image dimensions
 *          - channels: Number of color channels
 *          - src: Source image data
 *          - dst: Destination image data buffer
 *
 * @return NULL (standard pthread worker return value)
 *
 * @note The rotation is performed around the specified center point (cx, cy)
 * @note Pixels outside the source image boundaries are set to 0 (black)
 * @note Thread-safe when different workers process non-overlapping Y ranges
 */
static void *worker_rotate(void *p) {
  WorkArgs *a = (WorkArgs *)p;
  float cosA = cosf(a->ang_rad), sinA = sinf(a->ang_rad);
  float cx = a->cx, cy = a->cy;
  for (int y = a->y0; y < a->y1; y++) {
    for (int x = 0; x < a->width; x++) {
      float xd = x - cx, yd = y - cy;
      // inverse mapping
      float xs = cosA * xd + sinA * yd + cx;
      float ys = -sinA * xd + cosA * yd + cy;
      for (int c = 0; c < a->channels; c++) {
        unsigned char val = 0;
        if (xs >= 0 && xs < a->width && ys >= 0 && ys < a->height)
          val = bilinear(a->src, a->width, a->height, c, xs, ys);
        a->dst[y][x][c] = val;
      }
    }
  }
  return NULL;
}

/**
 * Rotates an image using multiple threads for concurrent processing.
 *
 * This function performs image rotation by the specified angle using multiple
 * worker threads to process different rows concurrently for improved
 * performance. The rotation is performed around the center of the image.
 *
 * @param src Pointer to the source image data (3D array:
 * [height][width][channels])
 * @param dst Pointer to the destination image data (3D array:
 * [height][width][channels])
 * @param width Width of the image in pixels
 * @param height Height of the image in pixels
 * @param channels Number of color channels (e.g., 3 for RGB, 4 for RGBA)
 * @param ang_deg Rotation angle in degrees (positive values rotate clockwise)
 * @param num_threads Number of worker threads to use for concurrent processing
 *
 * @return Returns the result code from the thread launching operation
 *
 * @note The rotation center is calculated as ((width-1)/2, (height-1)/2)
 * @note The angle is internally converted from degrees to radians for
 * computation
 */
int rotate_concurrent(unsigned char ***src, unsigned char ***dst, int width,
                      int height, int channels, float ang_deg,
                      int num_threads) {
  WorkArgs base = {.src = src,
                   .dst = dst,
                   .width = width,
                   .height = height,
                   .channels = channels};
  base.cx = (width - 1) / 2.0f;
  base.cy = (height - 1) / 2.0f;
  base.ang_rad = ang_deg * (float)M_PI / 180.0f;
  return launch_threads_by_rows(worker_rotate, base, num_threads);
}
