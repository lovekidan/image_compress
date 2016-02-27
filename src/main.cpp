#include <stdio.h>
#include <string.h>
#include "imageutil/image_util.hpp"

int main(int argc, char** argv) {
	//string image_path;
	printf("welcome to use image compress.\n");
	if (2 == argc)
	{
		printf("%s 's type is %d\n", argv[1], ic_get_image_type(argv[1]));
	}
	return 0;
}
