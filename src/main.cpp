#include <stdio.h>
#include <string.h>
#include "pngimage/png_image.hpp"

int main(int argc, char** argv) {
	//string image_path;
	printf("welcome to use image compress.\n");
	PngImage* png_image_ = new PngImage();
	printf("argc=%d\n", argc);
	if (2 == argc)
	{
		png_image_->read_png(argv[1]);
		printf("%s 's type is %d\n", argv[1], png_image_->getPngType());
	}
	return 0;
}
