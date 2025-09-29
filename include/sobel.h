#ifndef SOBEL_H
#define SOBEL_H
#include "utils_conc.h"

int sobel_concurrent(unsigned char ***src, unsigned char ***dst, int width,
                     int height, int channels, int num_threads);

#endif
