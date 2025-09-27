#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

// Define USE_STB if stb headers are provided under -Ithird_party
#ifdef USE_STB
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#endif

#include "utils_conc.h"

unsigned char*** crearMatriz3D(int alto, int ancho, int canales){
    // layout contiguo: [alto * ancho * canales] + tablas de punteros
    unsigned char* data = (unsigned char*)calloc((size_t)alto*ancho*canales, 1);
    if(!data) return NULL;
    unsigned char*** m = (unsigned char***)malloc(sizeof(unsigned char**)*alto);
    if(!m){ free(data); return NULL; }
    for(int y=0;y<alto;y++){
        m[y] = (unsigned char**)malloc(sizeof(unsigned char*)*ancho);
        if(!m[y]){ for(int i=0;i<y;i++) free(m[i]); free(m); free(data); return NULL; }
        for(int x=0;x<ancho;x++){
            m[y][x] = data + ((size_t)y*ancho + x)*canales;
        }
    }
    return m;
}

void liberarMatriz3D(unsigned char*** m){
    if(!m) return;
    unsigned char* base = NULL;
    // El primer píxel apunta al bloque contiguo
    if(m[0] && m[0][0]) base = m[0][0];
    int alto = 0; // no tenemos dims aquí; el caller debe conocerlas y liberar filas
    // No podemos conocer 'alto' desde aquí sin almacenarlo; suelta filas y bloque contiguo.
    // Asumimos patrón de creación: liberar filas hasta NULL y luego el bloque base.
    // Para simplificar, pedimos al caller liberar manualmente:
    //   for (int y=0;y<alto;y++) free(m[y]);
    //   free(m); free(base);
    // Para evitar errores, no hacemos nada aquí.
}

void copiar3D(unsigned char*** src, unsigned char*** dst, int ancho, int alto, int canales){
    for(int y=0;y<alto;y++){
        for(int x=0;x<ancho;x++){
            memcpy(dst[y][x], src[y][x], (size_t)canales);
        }
    }
}

int lanzar_hilos_por_filas(void* (*worker)(void*), WorkArgs base, int num_hilos){
    if(num_hilos < 1) num_hilos = 1;
    pthread_t* tids = (pthread_t*)malloc(sizeof(pthread_t)*num_hilos);
    WorkArgs* args = (WorkArgs*)malloc(sizeof(WorkArgs)*num_hilos);
    if(!tids || !args){ perror("malloc"); free(tids); free(args); return -1; }
    int filas = base.alto;
    int por = (int)ceil((double)filas / num_hilos);
    for(int i=0;i<num_hilos;i++){
        args[i] = base;
        args[i].y0 = i*por;
        args[i].y1 = args[i].y0 + por;
        if(args[i].y0 > filas) args[i].y0 = filas;
        if(args[i].y1 > filas) args[i].y1 = filas;
        if(pthread_create(&tids[i], NULL, worker, &args[i]) != 0){
            perror("pthread_create"); free(tids); free(args); return -1;
        }
    }
    for(int i=0;i<num_hilos;i++) pthread_join(tids[i], NULL);
    free(tids); free(args);
    return 0;
}

// ------- Opcional: I/O con stb --------
int cargarPNG(const char* path, unsigned char**** out_px, int* w, int* h, int* ch){
#ifndef USE_STB
    fprintf(stderr, "[WARN] cargarPNG requiere stb (define USE_STB e incluye headers)\n");
    return -1;
#else
    int x,y,c;
    unsigned char* data = stbi_load(path, &x, &y, &c, 0);
    if(!data){ fprintf(stderr, "Error cargando %s\n", path); return -1; }
    *w = x; *h = y; *ch = c;
    unsigned char*** m = crearMatriz3D(y, x, c);
    if(!m){ stbi_image_free(data); return -1; }
    size_t row_bytes = (size_t)x * c;
    for(int yy=0; yy<y; yy++){
        for(int xx=0; xx<x; xx++){
            memcpy(m[yy][xx], data + (size_t)(yy*x + xx)*c, (size_t)c);
        }
    }
    stbi_image_free(data);
    *out_px = m;
    return 0;
#endif
}

int guardarPNG(const char* path, unsigned char*** px, int w, int h, int ch){
#ifndef USE_STB
    fprintf(stderr, "[WARN] guardarPNG requiere stb (define USE_STB e incluye headers)\n");
    return -1;
#else
    unsigned char* flat = (unsigned char*)malloc((size_t)w*h*ch);
    if(!flat) return -1;
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            memcpy(flat + (size_t)(y*w + x)*ch, px[y][x], (size_t)ch);
        }
    }
    int ok = stbi_write_png(path, w, h, ch, flat, w*ch);
    free(flat);
    return ok ? 0 : -1;
#endif
}
