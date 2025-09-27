#ifndef ROTATE_H
#define ROTATE_H
#include "utils_conc.h"

int rotate_concurrente(unsigned char*** src, unsigned char*** dst,
                       int ancho, int alto, int canales,
                       float ang_deg, int num_hilos);

#endif
