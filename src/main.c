#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "utils_conc.h"
#include "conv.h"
#include "sobel.h"
#include "rotate.h"
#include "resize.h"

// Helpers para alocación y liberación completa (matriz + filas + bloque)
typedef struct {
    unsigned char*** m;
    unsigned char* contiguous;
    int w,h,c;
} Image3D;

static Image3D alloc_image3d(int w, int h, int c){
    Image3D img = {0};
    img.w=w; img.h=h; img.c=c;
    // bloque contiguo
    img.contiguous = (unsigned char*)calloc((size_t)w*h*c, 1);
    img.m = (unsigned char***)malloc(sizeof(unsigned char**)*h);
    for(int y=0;y<h;y++){
        img.m[y] = (unsigned char**)malloc(sizeof(unsigned char*)*w);
        for(int x=0;x<w;x++){
            img.m[y][x] = img.contiguous + ((size_t)y*w + x)*c;
        }
    }
    return img;
}
static void free_image3d(Image3D* img){
    if(!img || !img->m) return;
    for(int y=0;y<img->h;y++) free(img->m[y]);
    free(img->m);
    free(img->contiguous);
    img->m=NULL; img->contiguous=NULL;
}

static void flat_to_3d(unsigned char* flat, Image3D* im){
    for(int y=0;y<im->h;y++)
        for(int x=0;x<im->w;x++)
            memcpy(im->m[y][x], flat + (size_t)(y*im->w + x)*im->c, im->c);
}

static void print_menu(void){
    printf("\n== Reto 2 — Menú ==\n");
    printf("1) Convolución (blur 3x3)\n");
    printf("2) Sobel (bordes)\n");
    printf("3) Rotar (grados)\n");
    printf("4) Resize (nuevo ancho/alto)\n");
    printf("5) Guardar y salir\n");
    printf("Opción: ");
}

int main(int argc, char** argv){
    if(argc < 3){
        fprintf(stderr, "Uso: %s input.png output.png\n", argv[0]);
        fprintf(stderr, "Nota: para PNG necesitas stb y compilar con -DUSE_STB -Ithird_party\n");
    }

    // Cargar imagen
    unsigned char*** src = NULL;
    int w=0,h=0,c=0;
    if(argc >= 2 && cargarPNG(argv[1], &src, &w, &h, &c) != 0){
        fprintf(stderr, "No se pudo cargar %s. Puedes integrar tus propias funciones de I/O.\n", argv[1]);
        return 1;
    } else if(argc < 2){
        fprintf(stderr, "Continuando sin imagen cargada (solo demostración de menú)...\n");
        w=256; h=256; c=3;
        Image3D demo = alloc_image3d(w,h,c);
        src = demo.m;
        // patron simple
        for(int y=0;y<h;y++) for(int x=0;x<w;x++){ src[y][x][0]=x; src[y][x][1]=y; src[y][x][2]=128; }
    }

    // dst buffer
    Image3D dst = alloc_image3d(w,h,c);

    int salir = 0;
    while(!salir){
        print_menu();
        int op; if(scanf("%d", &op)!=1){ break; }
        if(op == 1){
            // Blur 3x3
            float k[9] = {1,1,1,1,1,1,1,1,1};
            for(int i=0;i<9;i++) k[i] /= 9.0f;
            conv_concurrente(src, dst.m, w,h,c, k, 3, 1.0f, 0.0f, 4);
            // swap
            unsigned char*** tmp = src; src = dst.m; dst.m = tmp;
        } else if(op == 2){
            sobel_concurrente(src, dst.m, w,h,c, 4);
            unsigned char*** tmp = src; src = dst.m; dst.m = tmp;
        } else if(op == 3){
            float ang; printf("Ángulo (grados): "); scanf("%f", &ang);
            rotate_concurrente(src, dst.m, w,h,c, ang, 4);
            unsigned char*** tmp = src; src = dst.m; dst.m = tmp;
        } else if(op == 4){
            int nw, nh; printf("Nuevo ancho: "); scanf("%d",&nw); printf("Nuevo alto: "); scanf("%d",&nh);
            Image3D out = alloc_image3d(nw, nh, c);
            resize_concurrente(src, w,h,c, out.m, nw, nh, 4);
            free_image3d(&dst);
            dst = out;
            // swap: queremos que src sea la nueva imagen redimensionada
            unsigned char*** tmp = src; src = dst.m; dst.m = tmp;
            w = nw; h = nh;
        } else if(op == 5){
            salir = 1;
        } else {
            printf("Opción inválida.\n");
        }
    }

    if(argc >= 3){
        if(guardarPNG(argv[2], src, w,h,c) != 0){
            fprintf(stderr, "No se guardó PNG (falta stb o integra tu guardado).\n");
        } else {
            printf("Guardado en %s\n", argv[2]);
        }
    } else {
        printf("Sugerencia: ejecuta con: ./reto2 in.png out.png\n");
    }

    // liberar
    // Ojo: 'src' puede apuntar a distintos contiguos según swaps; por simplicidad dejamos que el SO libere al salir.
    free_image3d(&dst);
    return 0;
}
