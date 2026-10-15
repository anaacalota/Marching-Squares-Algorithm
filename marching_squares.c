#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXSTRING 11 /* longest command ("INIT_CONTUR") + '\0' */
#define MAXNR 16 /* 16 contour patterns, each made of 4 x 4 = 16 pixels */

/*
 * Reads the PPM header (format, width, height, max color value) from stdin.
 * Returns 0 on success, 1 if the format is not P3.
 */
int read_image(char format[3], int *width, int *height, int *max_color)
{
	int ok = 0;

	scanf("%2s", format);
	if (strcmp(format, "P3") != 0) {
		ok = 1;
	}

	scanf("%d %d", width, height);
	scanf("%d", max_color);

	return ok;
}

/*
 * Frees a matrix of ints that has "height" rows: every row first,
 * then the array of row pointers.
 */
void free_pixels(int **pixels, int height)
{
	for (int i = 0; i < height; i++) {
		free(pixels[i]);
	}
	free(pixels);
}

/*
 * Allocates the pixel matrix (height x 3 * width) and fills it with the
 * values read from stdin. If an image is already loaded, it is freed first.
 * Returns the new matrix, or NULL on allocation failure/invalid input.
 */
int **read_pixels(int **pixels, int old_height, int width, int height,
				  int max_color)
{
	// get rid of the previously loaded image, if any
	if (pixels) {
		free_pixels(pixels, old_height);
	}
	pixels = malloc(height * sizeof(int *));

	if (!pixels) {
		printf("malloc failed for rows\n");
		return NULL;
	}

	for (int i = 0; i < height; i++) {
		pixels[i] = malloc(3 * width * sizeof(int));
		if (!pixels[i]) {
			printf("malloc failed for a row of rgb\n");
			free_pixels(pixels, i);
			return NULL;
		}
	}

	int nr_values = 0, ok = 0;
	int i = 0, j = 0;

	while (i < height) {
		while (j < width * 3) {
			int value;

			// check if the input is valid
			if (scanf("%d", &value) != 1) {
				printf("Eroare: eroare citire pixeli\n");
				ok = 1;
				break;
			}
			if (value < 0 || value > max_color) {
				printf("Eroare: valoare pixel necorespunzatoare\n");
				free_pixels(pixels, height);
				return NULL;
			}
			pixels[i][j] = value;
			j++;
			nr_values++;
		}
		if (ok == 1) {
			break;
		}
		i++;
		j = 0;
	}

	if (nr_values != height * width * 3) {
		free_pixels(pixels, height);
		return NULL;
	}

	return pixels;
}

/*
 * Prints the pixel matrix to stdout in PPM format.
 */
void print_pixels(char format[3], int max_color, int **pixels,
				  int width, int height)
{
	printf("%s\n", format);
	printf("%d %d\n", width, height);
	printf("%d\n", max_color);

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			printf("%d %d %d\n",
				   pixels[i][j * 3],
				   pixels[i][j * 3 + 1],
				   pixels[i][j * 3 + 2]);
		}
	}
}

/*
 * Prints the grid to stdout.
 */
void print_grid(int **relevant_grid, int height, int width)
{
	printf("Grid calculat cu succes [%d-%d]\n", height / 4, width / 4);

	for (int i = 0; i < height / 4; i++) {
		for (int j = 0; j < width / 4; j++) {
			printf("%d ", relevant_grid[i][j]);
		}
		printf("\n");
	}
}

/*
 * Enlarges the image 4 times on each dimension. Every original pixel is
 * duplicated into a 4 x 4 block of identical pixels. The old matrix is
 * freed and the new one is returned; width and height are updated.
 * If an allocation fails, the original image is returned unchanged.
 */
int **resize(int **pixels, int *width, int *height)
{
	int resize_factor = 4;
	int big_width = (*width) * resize_factor;
	int big_height = (*height) * resize_factor;

	int **new_pixels = malloc(big_height * sizeof(int *));
	if (!new_pixels) {
		printf("malloc failed\n");
		return pixels;
	}

	for (int i = 0; i < big_height; i++) {
		new_pixels[i] = malloc(3 * big_width * sizeof(int));
		if (!new_pixels[i]) {
			printf("malloc failed\n");
			return pixels;
		}
	}

	for (int i = 0; i < big_height; i++) {
		for (int j = 0; j < big_width; j++) {
			int old_i = i / resize_factor;
			int old_j = j / resize_factor;
			new_pixels[i][j * 3] = pixels[old_i][old_j * 3];
			new_pixels[i][j * 3 + 1] = pixels[old_i][old_j * 3 + 1];
			new_pixels[i][j * 3 + 2] = pixels[old_i][old_j * 3 + 2];
		}
	}

	free_pixels(pixels, *height);

	*width = big_width;
	*height = big_height;
	return new_pixels;
}

/*
 * Builds the binary grid from the brightness matrix.
 *
 * The grid has (height / 4 + 1) x (width / 4 + 1) nodes: one node for
 * every corner of the 4 x 4 cells (the +1 gives the last row and column).
 * The value of a node is the average between the brightness of the
 * corresponding pixel and the brightness of its neighbours; nodes on the
 * border / in the corners have fewer neighbours. The average is then
 * compared with the threshold: 1 if it is not above it, 0 otherwise.
 */
int **create_grid(int **brightness, int height, int width, int threshold)
{
	int a = 0, b = 0;
	int **relevant_grid = malloc((height / 4 + 1) * sizeof(int *));

	if (!relevant_grid) {
		printf("malloc failed\n");
		return NULL;
	}

	for (int i = 0; i < height / 4 + 1; i++) {
		relevant_grid[i] = malloc((width / 4 + 1) * sizeof(int));
		if (!relevant_grid[i]) {
			printf("malloc failed\n");
			return NULL;
		}
	}

	// walk through the pixels that correspond to grid nodes (every 4th one)
	for (int i = 0; i < height + 1; i += 4) {
		for (int j = 0; j < width + 1; j += 4) {
			if (i == 0 && j != 0 && j < height - 1) {
				relevant_grid[a][b] = (brightness[i][j] +
									   brightness[i + 1][j - 1] +
									   brightness[i + 1][j + 1]) / 3;
			} else if (i == 0 && j == 0) {
				relevant_grid[a][b] = (brightness[i][j] +
									   brightness[i + 1][j + 1]) / 2;
			} else if (i == 0 && j == height) {
				relevant_grid[a][b] = brightness[i + 1][j - 1];
			} else if (i == height && j == 0) {
				relevant_grid[a][b] = brightness[i - 1][j + 1];
			} else if (i == height && j == height) {
				relevant_grid[a][b] = brightness[i - 1][j - 1];
			} else if (j == 0 && i != 0 && i < height - 1) {
				relevant_grid[a][b] = (brightness[i][j] +
									   brightness[i - 1][j + 1] +
									   brightness[i + 1][j + 1]) / 3;
			} else if (j == height && i != 0 && i < height) {
				relevant_grid[a][b] = (brightness[i - 1][j - 1] +
									   brightness[i + 1][j - 1]) / 2;
			} else if (i == height && j != 0 && j < height) {
				relevant_grid[a][b] = (brightness[i - 1][j - 1] +
									   brightness[i - 1][j + 1]) / 2;
			} else {
				relevant_grid[a][b] = (brightness[i][j] +
									   brightness[i - 1][j - 1] +
									   brightness[i - 1][j + 1] +
									   brightness[i + 1][j - 1] +
									   brightness[i + 1][j + 1]) / 5;
			}
			if (relevant_grid[a][b] > threshold) {
				relevant_grid[a][b] = 0;
			} else {
				relevant_grid[a][b] = 1;
			}

			b++;
		}
		a++;
		b = 0;
	}

	return relevant_grid;
}

/*
 * GRID command: computes the brightness of every pixel (the average of its
 * R, G, B values) and builds the binary grid from it with threshold 200.
 * The previous grid (if any) is freed. The brightness matrix has one extra
 * row and column (initialised with 0) so that neighbours of the last
 * pixels can be read safely.
 */
int **grid(int **pixels, int width, int height, int **relevant_grid,
		   int old_height)
{
	int k = 0, threshold = 200, **brightness;

	if (relevant_grid) {
		free_pixels(relevant_grid, old_height / 4 + 1);
	}

	brightness = calloc(height + 1, sizeof(int *));
	if (!brightness) {
		printf("calloc failed\n");
	}

	for (int i = 0; i < height + 1; i++) {
		brightness[i] = calloc(width + 1, sizeof(int));
		if (!brightness[i]) {
			printf("calloc failed\n");
		}
	}

	// sum up every group of 3 values (R, G, B), then divide by 3
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < (width * 3); j++) {
			brightness[i][k] += pixels[i][j];
			if ((j + 1) % 3 == 0) {
				brightness[i][k] /= 3;
				k++;
			}
		}
		k = 0;
	}
	relevant_grid = create_grid(brightness, height, width, threshold);

	free_pixels(brightness, height + 1);
	return relevant_grid;
}

/*
 * Builds the lookup table of the 16 contour patterns (INIT_CONTUR).
 * Row `code` of the table is the 4 x 4 pattern (read row by row) used for
 * a cell whose corner code is `code`. Pixel values:
 * 255 - the contour line, 180 - occupied area, 0 - free area.
 */
int **init_contour(int **contours)
{
	if (contours) {
		free_pixels(contours, MAXNR);
	}

	contours = malloc(MAXNR * sizeof(int *));
	if (!contours) {
		printf("malloc failed\n");
		return NULL;
	}

	int values[MAXNR][MAXNR] = {
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 0, 180, 255, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0, 0, 255, 180},
		{0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 180, 180, 180, 180},
		{0, 0, 255, 180, 0, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 255, 180, 0, 0, 0, 255, 255, 0, 0, 0, 180, 255, 0, 0},
		{0, 0, 255, 180, 0, 0, 255, 180, 0, 0, 255, 180, 0, 0, 255, 180},
		{0, 255, 180, 180, 255, 180, 180, 180, 180, 180, 180, 180,
		 180, 180, 180, 180},
		{180, 255, 0, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{180, 255, 0, 0, 180, 255, 0, 0, 180, 255, 0, 0, 180, 255, 0, 0},
		{180, 255, 0, 0, 255, 0, 0, 0, 0, 0, 0, 255, 0, 0, 255, 180},
		{180, 180, 255, 0, 180, 180, 180, 255, 180, 180, 180, 180, 180,
		 180, 180, 180},
		{180, 180, 180, 180, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0},
		{180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180, 255, 180,
		 180, 255, 0},
		{180, 180, 180, 180, 180, 180, 180, 180, 255, 180, 180, 180, 0,
		 255, 180, 180},
		{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		 255, 255, 255}
	};

	// copy the static table into dynamically allocated memory
	for (int i = 0; i < MAXNR; i++) {
		contours[i] = malloc(MAXNR * sizeof(int));
		if (!contours[i]) {
			printf("malloc failed for row %d\n", i);
			for (int j = 0; j < i; j++) {
				free(contours[j]);
			}
			free(contours);
			return NULL;
		}

		for (int j = 0; j < MAXNR; j++) {
			contours[i][j] = values[i][j];
		}
	}

	return contours;
}

/*
 * Prints the 16 patterns as 4 x 4 PPM images (INIT_CONTUR output).
 * Each value of a pattern is printed 3 times (grayscale: R = G = B).
 */
void print_contours(char format[3], int max_color, int **contours)
{
	int size_contour = 4;

	for (int i = 0; i < MAXNR; i++) {
		printf("%s\n", format);
		printf("%d %d\n", size_contour, size_contour);
		printf("%d\n", max_color);

		for (int j = 0; j < MAXNR; j++) {
			for (int k = 0; k < 3; k++) {
				printf("%d ", contours[i][j]);
			}
			printf("\n");
		}
	}
}

/*
 * Applies the Marching Squares algorithm in place. For every 4 x 4 cell of
 * the image, the 4 grid values at its corners are combined into a code
 * between 0 and 15 (clockwise from the top-left corner, top-left being the
 * most significant bit). The cell is then overwritten with the contour
 * pattern that has that code in the lookup table.
 */
int **march(int **pixels, int height, int **relevant_grid, int **contours,
			int width)
{
	for (int i = 0; i < height / 4 ; i++) {
		for (int j = 0; j < width / 4; j++) {
			// transform from binary to decimal: top-left, top-right
			// bottom-right, bottom-left
			int code = 8 * relevant_grid[i][j] + 4 * relevant_grid[i][j + 1]
				+ 2 * relevant_grid[i + 1][j + 1] + relevant_grid[i + 1][j];

			int nr = 0;

			// replace the 4 x 4 block of the image with the corresponding contour pattern
			for (int q = 0; q < MAXNR / 4; q++) {
				for (int r = 0; r < MAXNR / 4; r++) {
					pixels[4 * i + q][3 * (4 * j + r)] =
						contours[code][nr];
					pixels[4 * i + q][3 * (4 * j + r) + 1] =
						contours[code][nr];
					pixels[4 * i + q][3 * (4 * j + r) + 2] =
						contours[code][nr++];
				}
			}
		}
	}

	return pixels;
}

int main(void)
{
	char command[MAXSTRING], format[3];
	int height = 0, width, **pixels = NULL;
	int max_color, **relevant_grid = NULL;
	int **contours = NULL;
	int old_height;

	while (1) {
		scanf("%s", command);

		if (strcmp(command, "READ") == 0) {
			old_height = height;

			if (read_image(format, &width, &height, &max_color) != 0) {
				pixels = read_pixels(pixels, old_height, width, height,
									 max_color);
				printf("Eroare: trebuie sa fie P3\n");
				if (pixels) {
					free_pixels(pixels, height);
				}
				return 0;
			}
			pixels = read_pixels(pixels, old_height, width, height, max_color);

			if (old_height == 0) {
				old_height = height;
				}

			if (!pixels) {
				return 0;
			}

			printf("Imagine citita cu succes [%d-%d]\n", width, height);
		} else if (strcmp(command, "WRITE") == 0) {
			if (pixels) {
				print_pixels(format, max_color, pixels, width, height);
			} else {
				printf("Eroare: Nu au fost cititi pixeli.\n");
			}
		} else if (strcmp(command, "RESIZE") == 0) {
			if (pixels) {
				pixels = resize(pixels, &width, &height);
			} else {
				printf("Eroare: Nu au fost cititi pixeli.\n");
			}
			printf("Imagine redimensionata cu succes [%d-%d]\n", height,
				   width);
		} else if (strcmp(command, "INIT_CONTUR") == 0) {
			contours = init_contour(contours);
			print_contours(format, max_color, contours);
		} else if (strcmp(command, "GRID") == 0) {
			relevant_grid = grid(pixels, width, height, relevant_grid,
								 old_height);
			print_grid(relevant_grid, height, width);
		} else if (strcmp(command, "MARCH") == 0) {
			relevant_grid = grid(pixels, width, height, relevant_grid,
								 old_height);
			contours = init_contour(contours);
			printf("Marching Squares aplicat cu succes [%d-%d]\n", height,
				   width);
			march(pixels, height, relevant_grid, contours, width);
		} else if (strcmp(command, "EXIT") == 0) {
			if (pixels) {
				free_pixels(pixels, height);
			}
			printf("Gigel a terminat\n");
			break;
		}
	}

	// free the remaining resources
	if (relevant_grid) {
		free_pixels(relevant_grid, height / 4 + 1);
	}
	if (contours) {
		free_pixels(contours, MAXNR);
	}

	return 0;
}
