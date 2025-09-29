#ifndef CONV_H
#define CONV_H
#include "utils_conc.h"

int conv_concurrent(unsigned char ***src, unsigned char ***dst, int width,
                    int height, int channels, const float *kernel, int k,
                    float factor, float bias, int num_threads);

#endif
