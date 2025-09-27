# Reto 2 — Procesamiento de imágenes concurrente (C + pthread)

Este scaffold contiene el **menú**, el **patrón de hilos** y las implementaciones de **Convolución**, **Sobel**, **Rotación** y **Resize** listas para integrarlas con tu base.
Funciona con una estructura de imagen `unsigned char***` (alto × ancho × canales).

## Cómo usarlo

Tienes **dos opciones**:

### Opción A — Integrarlo con **tu base existente** (recomendada)
1. Copia `include/` y `src/` dentro de tu proyecto.
2. En tu `main.c` (o archivo principal):
   - Incluye los headers:  
     ```c
     #include "utils_conc.h"
     #include "conv.h"
     #include "sobel.h"
     #include "rotate.h"
     #include "resize.h"
     ```
   - Usa tus propias funciones de **carga/guardado** (PNG con stb, etc.).
   - Usa `crearMatriz3D`/`liberarMatriz3D` para alocar destinos y `lanzar_hilos_por_filas(...)` para paralelizar.

### Opción B — Probarlo **standalone** con stb (rápido)
1. Descarga los headers de stb y colócalos en `third_party/`:
   - `stb_image.h`
   - `stb_image_write.h`
2. Compila con:
   ```bash
   gcc -O2 -Wall -Wextra -Iinclude -Ithird_party -o reto2        src/main.c src/conv.c src/sobel.c src/rotate.c src/resize.c -pthread -lm
   ```
3. Ejecuta:
   ```bash
   ./reto2 input.png output.png
   ```
   Sigue el **menú** interactivo. Puedes aplicar múltiples operaciones y guardar.

> Nota: Si no usas stb, puedes reemplazar `cargarPNG`/`guardarPNG` por tus funciones existentes sin tocar el resto.

## Estructura
```
reto2-concurrente/
├─ include/
│  ├─ utils_conc.h
│  ├─ conv.h
│  ├─ sobel.h
│  ├─ rotate.h
│  └─ resize.h
├─ src/
│  ├─ main.c
│  ├─ conv.c
│  ├─ sobel.c
│  ├─ rotate.c
│  └─ resize.c
└─ third_party/
   └─ (pon aquí stb_image.h y stb_image_write.h si usas la Opción B)
```

## Notas de implementación
- **Convolución**: soporta kernels 3×3/5×5 con padding por “clamp” y parámetros `factor` y `bias`.
- **Sobel**: calcula bordes sobre **luminancia** (si RGB) y deja salida en 1 o 3 canales.
- **Rotación**: mapeo inverso con **bilinear** (centro de imagen).
- **Resize**: escalado **bilinear** destino→origen (sin aliasing severo).
- **Hilos**: división por filas (`y0..y1`) con `pthread_create/join`.

## TODO opcionales
- Añadir kernels extra (sharpen, emboss, gaussian 5×5).
- Soporte in-place con **buffer swap** (src↔dst).
- Manejo de imágenes con alfa (RGBA).
