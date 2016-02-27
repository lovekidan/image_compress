#ifndef IC_UTIL
#define IC_UTIL

typedef enum _IMAGE_TYPE{
	IMAGE_TYPE_UNKNOWN = 0,
	IMAGE_TYPE_NOT_EXIST = 1,
	IMAGE_TYPE_JPEG = 2,
	IMAGE_TYPE_NORMAL_PNG = 3,
	IMAGE_TYPE_APNG = 4,
	IMAGE_TYPE_COMPLIED_9PNG = 5,
	IMAGE_TYPE_UNCOMPLIED_9PNG = 6,
	IMAGE_TYPE_GFT = 7
}IMAGE_TYPE;

// get image type
IMAGE_TYPE ic_get_image_type(const char *filePath);

// compress png image
bool ic_compress_png(const char* inputFile, const char* outputFile, int quality);

// compress apng image
bool ic_compress_apng(const char* inputFile, const char* outputFile);

// compress jpeg image
bool ic_compress_jpeg(const char* inputFile, const char* outputFile, int quality);

#endif