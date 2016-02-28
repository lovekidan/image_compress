#include <stdio.h>
#include <string.h>
#include "icutil/ic_util.hpp"

int main(int argc, char** argv) {
	//string image_path;
	printf("welcome to use image compress.\n");
	if (2 == argc)
	{
		IMAGE_TYPE image_type = ic_get_image_type(argv[1]);
		printf("%s 's type is %d\n", argv[1], image_type);
		switch (image_type) {
			case IMAGE_TYPE_NORMAL_PNG:
			ic_compress_png(argv[1], "/home/lovekid/Downloads/png_compressed.png", 30);
			break;
			case IMAGE_TYPE_APNG:
			ic_compress_apng(argv[1], "/home/lovekid/Downloads/apng_compressed.png");
			break;
			case IMAGE_TYPE_JPEG:
			ic_compress_jpeg(argv[1], "/home/lovekid/Downloads/jpeg_compressed.png", 80);
			break;
			case IMAGE_TYPE_UNCOMPLIED_9PNG:
			ic_compress_nine_patch_png(argv[1], "/home/lovekid/Downloads/jpeg_compressed.png", 80);
			break;
		}
	}

	return 0;
}
