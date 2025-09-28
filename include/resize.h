#ifndef RESIZE_H
#define RESIZE_H
#include "utils_conc.h"

int resize_concurrente(unsigned char*** src, int w, int h, int channels,
                       unsigned char*** dst, int nw, int nh,
                       int num_threads);

#endif
