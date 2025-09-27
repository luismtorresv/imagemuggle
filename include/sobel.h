#ifndef SOBEL_H
#define SOBEL_H
#include "utils_conc.h"

int sobel_concurrente(unsigned char*** src,
                      unsigned char*** dst,
                      int ancho, int alto, int canales,
                      int num_hilos);

#endif
