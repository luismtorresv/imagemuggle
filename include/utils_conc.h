#ifndef UTILS_CONC_H
#define UTILS_CONC_H
#include <pthread.h>

typedef struct {
    unsigned char*** src;   // lectura
    unsigned char*** dst;   // escritura
    int ancho, alto, canales;
    int y0, y1;             // rango [y0, y1)

    // Convolución
    const float* kernel; int k; float factor; float bias;
    // Rotación
    float cx, cy, ang_rad;
    // Resize
    float scale_x, scale_y;
} WorkArgs;

// Crea matriz 3D [alto][ancho][canales] contigua en memoria
unsigned char*** crearMatriz3D(int alto, int ancho, int canales);
// Libera creada por crearMatriz3D
void liberarMatriz3D(unsigned char*** m);
// Copia de src->dst (mismas dims)
void copiar3D(unsigned char*** src, unsigned char*** dst, int ancho, int alto, int canales);

// Lanza N hilos ejecutando 'worker' con división por filas
int lanzar_hilos_por_filas(void* (*worker)(void*), WorkArgs base, int num_hilos);

// Utilidades de I/O (stb opcional)
int cargarPNG(const char* path, unsigned char**** out_px, int* w, int* h, int* ch);
int guardarPNG(const char* path, unsigned char*** px, int w, int h, int ch);

#endif
