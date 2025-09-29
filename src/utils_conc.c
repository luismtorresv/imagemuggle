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

/**
 * Creates a 3D matrix with contiguous memory layout for image data
 *
 * Allocates memory for a 3D matrix structure that can be accessed as
 * m[y][x][channel]. The actual data is stored in a contiguous block for better
 * cache performance, while maintaining the convenience of 3D array indexing
 * through pointer tables.
 *
 * @param height Number of rows in the matrix
 * @param width Number of columns in the matrix
 * @param channels Number of channels per pixel (e.g., 3 for RGB, 4 for RGBA)
 *
 * @return Pointer to the 3D matrix structure on success, NULL on memory
 * allocation failure
 *
 * @note The returned matrix should be freed using a corresponding free3DMatrix
 * function
 * @note Memory layout: contiguous data block + height pointers + (height *
 * width) pointers
 * @warning Returns NULL if any memory allocation fails; all previously
 * allocated memory is cleaned up
 */
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

/**
 * Launches worker threads to process image data by dividing rows among threads
 *
 * This function creates multiple threads to process an image or data structure
 * by dividing the total number of rows evenly among the specified number of
 * threads. Each thread receives a copy of the base WorkArgs with modified y0
 * and y1 values to define its row processing range.
 *
 * @param worker Function pointer to the worker function that each thread will
 * execute. The function should accept a void* parameter (WorkArgs*) and return
 * void*.
 * @param base Base WorkArgs structure containing common parameters for all
 * threads. The height field is used to determine total rows to process.
 * @param num_threads Number of threads to create. If less than 1, defaults
 * to 1.
 *
 * @return 0 on success, -1 on failure (malloc error or pthread_create error)
 *
 * @note The function automatically handles row boundaries to ensure no thread
 *       processes rows beyond the total height.
 * @note All threads are joined before the function returns, ensuring completion
 *       of all work before cleanup.
 * @note Memory for thread IDs and arguments is automatically allocated and
 * freed.
 */
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

/**
 * Loads a PNG image from file and converts it to a 3D matrix format
 *
 * This function loads a PNG image using the stb_image library and converts the
 * linear pixel data into a 3D matrix where pixels can be accessed as
 * matrix[y][x][channel]. Requires USE_STB to be defined and stb_image headers
 * to be included.
 *
 * @param path Path to the PNG file to load
 * @param out_px Pointer to store the allocated 3D pixel matrix (y, x, channels)
 * @param w Pointer to store the image width
 * @param h Pointer to store the image height
 * @param ch Pointer to store the number of channels (e.g., 3 for RGB, 4 for
 * RGBA)
 *
 * @return 0 on success, -1 on failure (file not found, memory allocation error,
 *         or USE_STB not defined)
 *
 * @note The caller is responsible for freeing the allocated 3D matrix memory
 * @note Prints warning to stderr if USE_STB is not defined
 * @note Prints error to stderr if file cannot be loaded
 */
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

/**
 * Saves a 3D pixel array as a PNG image file
 *
 * This function converts a 3-dimensional pixel array into a flat buffer and
 * saves it as a PNG image using the STB image library. The function requires
 * STB to be available (USE_STB must be defined).
 *
 * @param path The file path where the PNG image will be saved
 * @param px 3D array of pixel data [height][width][channels]
 * @param w Width of the image in pixels
 * @param h Height of the image in pixels
 * @param ch Number of channels per pixel (e.g., 3 for RGB, 4 for RGBA)
 *
 * @return 0 on success, -1 on failure (memory allocation error, STB not
 * available, or PNG write failure)
 *
 * @note Requires STB image library to be compiled with USE_STB defined
 * @warning Function prints a warning to stderr if STB is not available
 */
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
