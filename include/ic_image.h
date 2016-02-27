#ifndef IC_IMAGE
#define IC_IMAGE

bool ApngCompress(const char* inputFile, const char* outputFile);

bool ApngDissemble(const char* inputFile, const char* outputDir);

bool ApngAssemble(const char *inputDir, const char* outputFile);

bool NinePatchOpt(const char *inputFile, const char *outputFile, int outputKind, int(&ninePatch)[4], int quality = 100);

bool PngZopfliCompress(const char *inputFile, const char*outputFile);

extern "C"{
    int PngQuantFile(const char *fileName, const char *outName, int quality);
    int CJpegFile(const char *fileName, const char *outNamem, int quality);
}

#endif