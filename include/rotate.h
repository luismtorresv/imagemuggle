#ifndef ROTATE_H
#define ROTATE_H
#include "utils_conc.h"

int rotate_concurrent(unsigned char ***src, unsigned char ***dst, int width,
                      int height, int channels, float ang_deg, int num_threads);

#endif
