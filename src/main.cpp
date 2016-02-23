#include <stdio.h>
#include "pngimage/png_image.hpp"

int main(int argc, char** argv) {
	printf("welcome to use image compress.\n");
	PngImage png_image;
	png_image.read_png("");
	return 0;
}
