Copyright 2024 Ana-Cristina Calota

# Marching Squares

## Overview

A C program that works on PPM (P3) images. It reads an image from standard
input, enlarges it, computes a binary grid from its brightness and applies the
Marching Squares algorithm to draw contour patterns on it. The image is stored
in dynamically allocated memory and commands are read until `EXIT`.

Commands: `READ`, `WRITE`, `RESIZE`, `INIT_CONTUR`, `GRID`, `MARCH`, `EXIT`.

## Implementation Details

* **Image storage:** the image is a `height x (3 * width)` matrix of `int`;
  every pixel takes 3 consecutive values (R, G, B) in its row.
* **Reading:** `read_image()` reads the header (format, width, height, max
  color) and checks that the format is `P3`. `read_pixels()` allocates the
  matrix and reads every value, checking that it is between 0 and `max_color`.
  On invalid input it prints an error and frees the allocated memory. When a
  new image is read, the previous matrix is freed first.
* **Printing:** `print_pixels()` prints the initial or the modified image.
* **Resizing:** `resize()` allocates a new matrix 4 times larger on both
  dimensions and duplicates every pixel into a 4x4 block. The old matrix is
  freed afterwards.
* **Grid:** `grid()` first computes the brightness of every pixel (the average
  of R, G, B) into a temporary matrix. `create_grid()` then builds
  `relevant_grid`, of size `(height / 4 + 1) x (width / 4 + 1)`, where each
  node is the average between the brightness of the pixel and of its neighbours
  taken in an X shape (fewer on borders and corners). The extra last row and
  column are computed only from neighbours. A node becomes 1 if its value is
  not above the threshold (200) and 0 otherwise.
* **Contours:** `init_contour()` stores the 16 contour patterns (4x4 pixels,
  values 0, 180 and 255) in the `contours` matrix, one row per pattern.
  `INIT_CONTUR` prints all of them.
* **Marching Squares:** `march()` goes through the grid cell by cell. For each
  cell it converts the 4 corner bits, taken clockwise from the top-left corner,
  into a number between 0 and 15 and overwrites the corresponding 4x4 block of
  the image with the pattern stored at that index.
* **Memory management:** all matrices are allocated with `malloc` / `calloc`
  and released with `free_pixels()`. The image, the grid and the contour table
  are freed when they are replaced and at `EXIT`.

## Conclusion

The program applies Marching Squares on PPM images using only dynamically
allocated matrices, with a separate function for each step of the algorithm.