#include <stdio.h>
#include "pngimage/png_image.hpp"

int main(int argc, char** argv) {
	std::string image_path = "/XX";
	printf("welcome to use image compress.\n");
	PngImage* png_image_ = new PngImage();
	png_image_->read_png(image_path);
	return 0;
}
