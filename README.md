# `imagemuggle` - Image Processing Concurrent Extension in C

A concurrent image processing application implementing convolution, Sobel edge
detection, rotation, and bilinear scaling operations using POSIX threads. The
system processes PNG images through a 3D matrix structure (`unsigned char***`)
with multi-threaded row-based parallelization.

## Features

- **Convolution**: Applies 3×3/5×5 kernels with clamp padding, configurable
  factor and bias parameters. Multiple application modes (light, medium, heavy)
  for enhanced blur effects through iterative convolution
- **Sobel Edge Detection**: Computes gradients on luminance (RGB) with single or
  three-channel output
- **Image Rotation**: Inverse mapping with bilinear interpolation around image
  center
- **Bilinear Scaling**: Destination-to-source scaling without severe aliasing
- **Thread Management**: Row-based division (`y0..y1`) using
  `pthread_create/join`

## Installation

### Requirements
- GCC compiler with C99 support
- POSIX threads library
- Math library (`-lm`)
- STB image libraries (included)

### Build

```bash
make
```

## Usage

```bash
./imagemuggle input.png output.png
```

Interactive menu options:
1. Convolution (kernel size, factor, bias)
2. Sobel edge detection
3. Rotation (angle in degrees)
4. Resize (new width/height)
5. Exit

Multiple operations can be applied sequentially before saving.

## Project structure

```
include/
├── utils_conc.h    # Threading utilities, 3D matrix operations
├── conv.h          # Convolution operations
├── sobel.h         # Edge detection
├── rotate.h        # Image rotation
└── resize.h        # Bilinear scaling

src/
├── main.c          # Interactive menu, image I/O
├── utils_conc.c    # Matrix allocation, threading framework
├── conv.c          # Kernel convolution implementation
├── sobel.c         # Sobel operator implementation
├── rotate.c        # Geometric transformation
└── resize.c        # Bilinear interpolation

third_party/
├── stb_image.h
└── stb_image_write.h
```

## Implementation Details

All operations use the `WorkArgs` structure for thread communication and employ
`launch_threads_by_rows()` for parallelization. The system supports both
grayscale and RGB images with dynamic memory management through
`create3DMatrix()` and proper cleanup procedures.

Thread distribution divides image rows among worker threads, with each thread
processing a contiguous range `[y0, y1)`. Synchronization occurs through
`pthread_join()` without requiring mutexes due to non-overlapping memory
regions.

## Project Status

Active development. Core functionality implemented and tested. Potential
extensions include additional kernel types, in-place operations with buffer
swapping, and RGBA channel support.
