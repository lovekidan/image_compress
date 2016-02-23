#ifndef PNG_IMAGE
#define PNG_IMAGE

#include <opencv.hpp>
#include "libpng/png.h"
#include "zlib/zlib.h"

class PngImage{
public:
    PngImage() :rows_buf(NULL){ release(); }
    ~PngImage(){ release(); }

    bool read_png(const std::string &fileName);
    bool read_png_from_Mat(const cv::Mat &mat);
    bool write_png(const std::string &fileName);
    cv::Mat get_Mat_from_png();

    enum PNG_IMAGE_TYPE{
        PNG_IMAGE_TYPE_NOT_EXIST,
        PNG_IMAGE_TYPE_UNKNOWN,
        PNG_IMAGE_TYPE_NORMAL,
        PNG_IMAGE_TYPE_APNG,
        PNG_IMAGE_TYPE_UNCOMPLIED_9PNG,
        PNG_IMAGE_TYPE_COMPLIED_9PNG
    };
    PNG_IMAGE_TYPE getPngType();
    //left top right bottom
    bool set_npTc_info(const cv::Vec4i &gird, const cv::Vec4i &padding);
    bool set_de9patch();

    void release();

private:
    bool write_npTc_chunk(png_structp &write_ptr, png_infop &write_info_ptr);
    bool isUnComplied9Png();
    unsigned int get_nine_patch_color(png_bytepp rows, int left, int top, int right, int bottom);
    bool have_image;
    int bytes_per_pixel;

    //IHDR
    bool have_IHDR;
    png_uint_32 width, height;
    int bit_depth, color_type;
    int interlace_type;
    double gamma;

    //Pixels
    png_bytepp rows_buf;

    //npTc
#pragma pack(push,4)
    typedef struct npTc_block_t {
        char wasDeserialized;
        char numXDivs;
        char numYDivs;
        char numColors;
        unsigned int xDivsOffset;
        unsigned int yDivsOffset;
        int padding_left;
        int padding_right;
        int padding_top;
        int padding_bottom;
        unsigned int colorsOffset;
        int XDivs[2];
        int YDivs[2];
        unsigned int colors[9];

        size_t get_npTc_size();
        bool serialize(char *data);
    } npTc_block;
#pragma pack(pop)
    bool have_npTc;
    npTc_block npTc;

    void swap_columns(int col1, int col2);
    void add_borders();
    void add_patches(npTc_block * np_block);
    void add_paddings(npTc_block * np_block);

    //unknown
    PNG_IMAGE_TYPE png_type;
};

#endif