#include "resize.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * Arguments structure for image resize operations
 *
 * This structure extends the base WorkArgs structure with additional
 * parameters specific to resize operations, including the target dimensions.
 *
 * @param a Base work arguments structure containing common operation parameters
 * @param nw New width for the resized image
 * @param nh New height for the resized image
 */
typedef struct {
  WorkArgs a;
  int nw, nh;
} ResizeArgs;

/**
 * Performs bilinear interpolation to sample a pixel value from a 3D image
 * array.
 *
 * This function implements bilinear interpolation to calculate the pixel value
 * at non-integer coordinates by interpolating between the four nearest pixels.
 * The function handles boundary conditions by clamping coordinates to valid
 * ranges.
 *
 * @param src    3D array representing the image data [height][width][channels]
 * @param w      Width of the image in pixels
 * @param h      Height of the image in pixels
 * @param c      Channel index to sample from (e.g., 0=R, 1=G, 2=B for RGB)
 * @param xs     X-coordinate to sample (can be fractional)
 * @param ys     Y-coordinate to sample (can be fractional)
 *
 * @return       Interpolated pixel value as an unsigned char (0-255)
 *
 * @note         Coordinates outside image bounds are clamped to nearest valid
 * pixel
 * @note         Uses lrintf() for proper rounding of final interpolated value
 */
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

/**
 * Worker thread function for image resizing using bilinear interpolation.
 *
 * This function performs bilinear interpolation to resize a portion of an
 * image. It calculates scaling factors and maps destination pixels to source
 * coordinates, then applies bilinear interpolation for each color channel.
 *
 * @param p Pointer to ResizeArgs structure containing:
 *          - a: WorkArgs with source/destination buffers, dimensions, and work
 * range
 *          - nw: New width (destination width)
 *          - nh: New height (destination height)
 *
 * @return NULL (standard pthread worker return value)
 *
 * @note The function processes rows from a->y0 to a->y1 (exclusive) for
 * parallelization
 * @note Uses center-point sampling with 0.5 pixel offset for better
 * interpolation
 * @note Assumes bilinear() function is available for pixel interpolation
 */
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

/**
 * Resizes an image using multiple threads for concurrent processing.
 *
 * This function performs image resizing by distributing the work across
 * multiple threads, where each thread processes a portion of the output image
 * rows. The image is resized from the source dimensions (w x h) to the new
 * dimensions (nw x nh).
 *
 * @param src Pointer to 3D array containing source image data
 * [height][width][channels]
 * @param w Width of the source image in pixels
 * @param h Height of the source image in pixels
 * @param channels Number of color channels in the image (e.g., 3 for RGB, 4 for
 * RGBA)
 * @param dst Pointer to 3D array where resized image data will be stored
 * [nh][nw][channels]
 * @param nw Target width of the resized image in pixels
 * @param nh Target height of the resized image in pixels
 * @param num_threads Number of worker threads to use for concurrent processing
 *
 * @return 0 on success, -1 on failure (memory allocation error or thread
 * creation failure)
 *
 * @note The destination image buffer must be pre-allocated before calling this
 * function
 * @note If thread creation fails, all previously created threads are properly
 * joined
 * @note Work is distributed evenly among threads with remainder rows assigned
 * to first threads
 */
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
