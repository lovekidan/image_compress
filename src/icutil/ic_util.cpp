#include "ic_util.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "boost/filesystem.hpp"
#include "pngimage/png_image.hpp"
#include "include/ic_image.h"

IMAGE_TYPE ic_get_image_type(const char *filePath){
    FILE *file = 0;
    IMAGE_TYPE result = IMAGE_TYPE_UNKNOWN;
    if ((file = fopen(filePath, "rb")) == 0)
        return IMAGE_TYPE_NOT_EXIST;
    unsigned char header[8] = { 0 };
    if (fread(header, 8, 1, file)){
        if (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF) {
            result = IMAGE_TYPE_JPEG;
        } else if (header[0] == 0x47 && header[1] == 0x49 &&
            header[2] == 0x46 && header[3] == 0x38) { 
            result = IMAGE_TYPE_GIF;
        } else if (header[0] == 0x89 && header[1] == 0x50 &&
            header[2] == 0x4E && header[3] == 0x47 &&
            header[4] == 0x0D && header[5] == 0x0A &&
            header[6] == 0x1A && header[7] == 0x0A){
            fclose(file);
            PngImage pngImage;
            pngImage.read_png(std::string(filePath));
            switch (pngImage.getPngType()){
            case PngImage::PNG_IMAGE_TYPE_APNG:
                return IMAGE_TYPE_APNG;
            case PngImage::PNG_IMAGE_TYPE_COMPLIED_9PNG:
                return IMAGE_TYPE_COMPLIED_9PNG;
            case PngImage::PNG_IMAGE_TYPE_NORMAL:
                return IMAGE_TYPE_NORMAL_PNG;
            case PngImage::PNG_IMAGE_TYPE_NOT_EXIST:
                return IMAGE_TYPE_NOT_EXIST;
            case PngImage::PNG_IMAGE_TYPE_UNCOMPLIED_9PNG:
                return IMAGE_TYPE_UNCOMPLIED_9PNG;
            case PngImage::PNG_IMAGE_TYPE_UNKNOWN:
                return IMAGE_TYPE_UNKNOWN;
            default:
                return IMAGE_TYPE_UNKNOWN;
            }
        }
        else {
            result = IMAGE_TYPE_UNKNOWN;
        }
    }
    else {
        result = IMAGE_TYPE_UNKNOWN;
    }
    fclose(file);
    return result;
}

bool ic_compress_png(const char* inputFile, const char* outputFile, int quality) {
    bool isPngQuantFileSuccess = PngQuantFile(inputFile, outputFile, quality);
    return PngZopfliCompress(isPngQuantFileSuccess ? inputFile : outputFile, outputFile);
}

bool ic_compress_apng(const char* inputFile, const char* outputFile) {
    return ApngCompress(inputFile, outputFile);
}

bool ic_compress_nine_patch_png(const char* inputFile, const char* outputFile, int quality) {
    bool result = false;
    if (NinePatchOpt(inputFile, outputFile, 2, 100)){
        result = PngQuantFile(outputFile, outputFile, quality) != 0;
    }
    else{
        result = PngQuantFile(inputFile, outputFile, quality) != 0;
    }

    if (result && ic_get_image_type(outputFile) == IMAGE_TYPE_UNCOMPLIED_9PNG){
        result = PngZopfliCompress(outputFile, outputFile);
    }
    else{
        result = PngZopfliCompress(inputFile, outputFile);
    }
    return result;
}

bool ic_compress_jpeg(const char* inputFile, const char* outputFile, int quality) {
    bool isNeedRename = 0 == strcmp(inputFile, outputFile);
    std::string tmpOutputFile(outputFile);
    if (isNeedRename)
    {
        tmpOutputFile.append(".tmp");
    }

    int result = CJpegFile(inputFile, tmpOutputFile.c_str(), quality);
    if(0 == result) {
        if (isNeedRename) {
            boost::filesystem::path destPath(outputFile);
            boost::filesystem::path currentPath(tmpOutputFile.c_str());
            try {
                boost::filesystem::rename(currentPath, destPath);
            } catch(boost::filesystem::filesystem_error e) {
                printf("error -> %s.\n", e.what());
                return false;
            }
        }
        return true;
    }

    printf("error -> compress fail, result code is %d.\n", result);
    
    return false;
}