#include "conv.h"
#include "resize.h"
#include "rotate.h"
#include "sobel.h"
#include "utils_conc.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helpers for full allocation and deallocation (matrix + rows + block)
typedef struct {
  unsigned char ***m;
  unsigned char *contiguous;
  int w, h, c;
} Image3D;

static Image3D alloc_image3d(int w, int h, int c) {
  Image3D img = {0};
  img.w = w;
  img.h = h;
  img.c = c;
  // contiguous block
  img.contiguous = (unsigned char *)calloc((size_t)w * h * c, 1);
  img.m = (unsigned char ***)malloc(sizeof(unsigned char **) * h);
  for (int y = 0; y < h; y++) {
    img.m[y] = (unsigned char **)malloc(sizeof(unsigned char *) * w);
    for (int x = 0; x < w; x++) {
      img.m[y][x] = img.contiguous + ((size_t)y * w + x) * c;
    }
  }
  return img;
}
static void free_image3d(Image3D *img) {
  if (!img || !img->m)
    return;
  for (int y = 0; y < img->h; y++)
    free(img->m[y]);
  free(img->m);
  free(img->contiguous);
  img->m = NULL;
  img->contiguous = NULL;
}

static void flat_to_3d(unsigned char *flat, Image3D *im) {
  for (int y = 0; y < im->h; y++)
    for (int x = 0; x < im->w; x++)
      memcpy(im->m[y][x], flat + (size_t)(y * im->w + x) * im->c, im->c);
}

static void print_menu(void) {
  printf("\nImage Processing Menu\n");
  printf("1) Convolution (3x3 blur)\n");
  printf("  1a) Light blur\n");
  printf("  1b) Medium blur\n");
  printf("  1c) Heavy blur\n");
  printf("2) Sobel edge detection\n");
  printf("3) Rotate (degrees)\n");
  printf("4) Resize (new width/height)\n");
  printf("5) Save and exit\n");
  printf("Option: ");
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s input.png output.png\n", argv[0]);
    fprintf(stderr, "Note: PNG support requires stb headers and compile with "
                    "-DUSE_STB -Ithird_party\n");
  }

  // Load image
  unsigned char ***src = NULL;
  int w = 0, h = 0, c = 0;
  if (argc >= 2 && loadPNG(argv[1], &src, &w, &h, &c) != 0) {
    fprintf(stderr,
            "Could not load %s. You can integrate your own I/O functions.\n",
            argv[1]);
    return 1;
  } else if (argc < 2) {
    fprintf(stderr,
            "Continuing without loaded image (menu demonstration only)...\n");
    w = 256;
    h = 256;
    c = 3;
    Image3D demo = alloc_image3d(w, h, c);
    src = demo.m;
    // simple pattern
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++) {
        src[y][x][0] = x;
        src[y][x][1] = y;
        src[y][x][2] = 128;
      }
  }

  // dst buffer
  Image3D dst = alloc_image3d(w, h, c);

  int exit_flag = 0;
  while (!exit_flag) {
    print_menu();
    int op;
    if (scanf("%d", &op) != 1) {
      break;
    }
    if (op == 1) {
      // Blur options
      printf("Choose blur strength:\n");
      printf("  a) Light blur (1x)\n");
      printf("  b) Medium blur (3x applications)\n");
      printf("  c) Heavy blur (5x applications)\n");
      printf("Choice (a/b/c): ");
      char choice;
      scanf(" %c", &choice);

      float k[9] = {1, 1, 1, 1, 1, 1, 1, 1, 1};
      for (int i = 0; i < 9; i++)
        k[i] /= 9.0f;

      int applications = 1;
      switch (choice) {
      case 'a':
        applications = 1;
        break;
      case 'b':
        applications = 3;
        break;
      case 'c':
        applications = 10;
        break;
      default:
        printf("Invalid choice, using light blur\n");
        applications = 1;
      }

      // Apply blur multiple times for stronger effect
      for (int i = 0; i < applications; i++) {
        conv_concurrente(src, dst.m, w, h, c, k, 3, 1.0f, 0.0f, 4);
        // swap buffers
        unsigned char ***tmp = src;
        src = dst.m;
        dst.m = tmp;
      }
      printf("Applied blur %d time(s)\n", applications);
    } else if (op == 2) {
      sobel_concurrente(src, dst.m, w, h, c, 4);
      unsigned char ***tmp = src;
      src = dst.m;
      dst.m = tmp;
    } else if (op == 3) {
      float ang;
      printf("Angle (degrees): ");
      scanf("%f", &ang);
      rotate_concurrente(src, dst.m, w, h, c, ang, 4);
      unsigned char ***tmp = src;
      src = dst.m;
      dst.m = tmp;
    } else if (op == 4) {
      int nw, nh;
      printf("New width: ");
      scanf("%d", &nw);
      printf("New height: ");
      scanf("%d", &nh);
      Image3D out = alloc_image3d(nw, nh, c);
      resize_concurrente(src, w, h, c, out.m, nw, nh, 4);
      free_image3d(&dst);
      dst = out;
      // swap: we want src to be the new resized image
      unsigned char ***tmp = src;
      src = dst.m;
      dst.m = tmp;
      w = nw;
      h = nh;
    } else if (op == 5) {
      exit_flag = 1;
    } else {
      printf("Invalid option.\n");
    }
  }

  if (argc >= 3) {
    if (savePNG(argv[2], src, w, h, c) != 0) {
      fprintf(stderr,
              "PNG not saved (missing stb or integrate your save function).\n");
    } else {
      printf("Saved to %s\n", argv[2]);
    }
  } else {
    printf("Suggestion: run with ./reto2 input.png output.png\n");
  }

  // cleanup
  // Note: 'src' may point to different contiguous blocks due to swaps; for
  // simplicity let OS free on exit.
  free_image3d(&dst);
  return 0;
}
