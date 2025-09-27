#ifndef CONV_H
#define CONV_H
#include "utils_conc.h"

int conv_concurrente(unsigned char*** src,
                     unsigned char*** dst,
                     int ancho, int alto, int canales,
                     const float* kernel, int k,
                     float factor, float bias,
                     int num_hilos);

#endif
