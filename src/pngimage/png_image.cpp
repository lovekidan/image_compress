#include "png_image.hpp"

inline static unsigned long int BigLittleSwap32(unsigned long int A){
    return ((((unsigned long int)(A)& 0xff000000) >> 24) |
        (((unsigned long int)(A)& 0x00ff0000) >> 8) |
        (((unsigned long int)(A)& 0x0000ff00) << 8) |
        (((unsigned long int)(A)& 0x000000ff) << 24));
}

inline static int checkCPUendian(){
    union{
        unsigned long int i;
        unsigned char s[4];
    }c;
    c.i = 0x12345678;
    return (0x12 == c.s[0]);
}

inline static unsigned long int t_htonl(unsigned long int h){
    return checkCPUendian() ? h : BigLittleSwap32(h);
}

inline static unsigned long int t_ntohl(unsigned long int n){
    return checkCPUendian() ? n : BigLittleSwap32(n);
}

PngImage::PngImage():rows_buf(NULL) {
    release();
}

PngImage::~PngImage() {
    release();
}

size_t PngImage::npTc_block::get_npTc_size(){
    return (sizeof(char) * 4 + sizeof(unsigned int)*(3 + numColors) + sizeof(int)*(4 + numXDivs + numYDivs));
}

bool PngImage::npTc_block::serialize(char *data){
    if (data == NULL){
        return false;
    }
    else{
        size_t pos = 0;
        data[pos] = wasDeserialized;
        pos++;
        data[pos] = numXDivs;
        pos++;
        data[pos] = numYDivs;
        pos++;
        data[pos] = numColors;
        pos++;
        *((unsigned int*)(data + pos)) = t_htonl(xDivsOffset);
        pos += sizeof(unsigned int);
        *((unsigned int*)(data + pos)) = t_htonl(yDivsOffset);
        pos += sizeof(unsigned int);
        *((int*)(data + pos)) = t_htonl(padding_left);
        pos += sizeof(int);
        *((int*)(data + pos)) = t_htonl(padding_right);
        pos += sizeof(int);
        *((int*)(data + pos)) = t_htonl(padding_top);
        pos += sizeof(int);
        *((int*)(data + pos)) = t_htonl(padding_bottom);
        pos += sizeof(int);
        *((unsigned int*)(data + pos)) = t_htonl(colorsOffset);
        pos += sizeof(unsigned int);
        for (int i = 0; i < numXDivs; i++){
            *((int*)(data + pos)) = t_htonl(XDivs[i]);
            pos += sizeof(int);
        }
        for (int i = 0; i < numYDivs; i++){
            *((int*)(data + pos)) = t_htonl(YDivs[i]);
            pos += sizeof(int);
        }
        for (int i = 0; i < numColors; i++){
            *((unsigned int*)(data + pos)) = t_htonl(colors[i]);
            pos += sizeof(unsigned int);
        }
        return true;
    }
}

static int read_chunk_custom(png_structp ptr, png_unknown_chunkp chunk){
    PngImage::PNG_IMAGE_TYPE *png_type = (PngImage::PNG_IMAGE_TYPE*)png_get_user_chunk_ptr(ptr);
    if (!strcmp((char*)chunk->name, "npTc")){
        *png_type = PngImage::PNG_IMAGE_TYPE_COMPLIED_9PNG;
        return 10;
    }
    if (!strcmp((char*)chunk->name, "acTL")||
        !strcmp((char*)chunk->name, "fcTL")||
        !strcmp((char*)chunk->name, "fdAT")){
        *png_type = PngImage::PNG_IMAGE_TYPE_APNG;
        return 10;
    }
    return 0;
}

bool PngImage::read_png(const std::string &fileName){
    release();
    FILE* fpin;
    if ((fpin = fopen(fileName.c_str(), "rb")) == NULL){
        fprintf(stderr, "Could not find input file %s\n", fileName.c_str());
        return false;
    }
    png_structp read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (read_ptr == NULL){
        fclose(fpin);
        return false;
    }
    png_infop read_info_ptr = png_create_info_struct(read_ptr);
    if (read_info_ptr == NULL){
        fclose(fpin);
        png_destroy_read_struct(&read_ptr, NULL, NULL);
        return false;
    }
    if (setjmp(png_jmpbuf(read_ptr))){
        png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
        fclose(fpin);
        return false;
    }
    png_init_io(read_ptr, fpin);
    png_set_sig_bytes(read_ptr, 0);
    png_type = PNG_IMAGE_TYPE_NORMAL;
    png_set_read_user_chunk_fn(read_ptr, &png_type, (png_user_chunk_ptr)read_chunk_custom);
    png_read_info(read_ptr, read_info_ptr);

    have_IHDR = png_get_IHDR(read_ptr, read_info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL) != 0;
    
    if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
        png_set_expand(read_ptr);
        png_set_filler(read_ptr, 65535L, PNG_FILLER_AFTER);
    }
    if (bit_depth == 16) {
        png_set_strip_16(read_ptr);
    }
    if (!(color_type & PNG_COLOR_MASK_COLOR)) {
        png_set_gray_to_rgb(read_ptr);
    }
    double _gamma = 0.45455;
    if (!png_get_valid(read_ptr, read_info_ptr, PNG_INFO_sRGB)) {
        png_get_gAMA(read_ptr, read_info_ptr, &gamma);
        if (gamma < 0 || gamma > 1.0) 
            gamma = 0.45455;
    }
    gamma = _gamma;

    bit_depth = 8;
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    bytes_per_pixel = 4;
    png_read_update_info(read_ptr, read_info_ptr);
    rows_buf = (png_bytepp)malloc(sizeof(png_bytep)*height);
    memset(rows_buf, 0, sizeof(png_bytep)*height);
    size_t rowbytes = png_get_rowbytes(read_ptr, read_info_ptr);
    if (rowbytes != width * bytes_per_pixel){
        png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
        fclose(fpin);
        return false;
    }
    for (size_t y = 0; y < height; y++)
        rows_buf[y] = (png_bytep)malloc(rowbytes);
    png_read_image(read_ptr, rows_buf);//RGBA
    have_image = true;
    isUnComplied9Png();
    png_read_end(read_ptr, read_info_ptr);
    png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);
    fclose(fpin);
    return true;
}

bool PngImage::read_png_from_Mat(const cv::Mat &mat){
    release();
    if (mat.data == NULL)
        return false;
    width = mat.cols;
    height = mat.rows;
    bit_depth = 8;
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    interlace_type = PNG_INTERLACE_NONE;
    have_IHDR = true;
    
    rows_buf = (png_bytepp)malloc(sizeof(png_bytep)*height);
    memset(rows_buf, 0, sizeof(png_bytep)*height);
    for (size_t y = 0; y < height; y++)
        rows_buf[y] = (png_bytep)malloc(width * 4);

    for (unsigned int i = 0; i < height; ++i) {
        for (unsigned int j = 0; j < width; ++j) {
            const cv::Vec4b &rgba = mat.at<cv::Vec4b>(i, j);
            rows_buf[i][j * 4 + 2] = rgba[0];//B
            rows_buf[i][j * 4 + 1] = rgba[1];//G
            rows_buf[i][j * 4] = rgba[2];//R
            rows_buf[i][j * 4 + 3] = rgba[3];//A
        }
    }
    png_type = PNG_IMAGE_TYPE_NORMAL;
    have_image = true;
    bytes_per_pixel = 4;
    isUnComplied9Png();
    return true;
}

bool PngImage::write_png(const std::string &fileName){
    if (!have_image){
        return false;
    }
    static FILE* fpout;
    if ((fpout = fopen(fileName.c_str(), "wb")) == NULL){
        return false;
    }
    png_structp write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (write_ptr == NULL){
        fclose(fpout);
        return false;
    }
    png_infop write_info_ptr = png_create_info_struct(write_ptr);
    if (write_info_ptr == NULL){
        fclose(fpout);
        png_destroy_write_struct(&write_ptr, NULL);
        return false;
    };
    if (setjmp(png_jmpbuf(write_ptr))){
        fclose(fpout);
        png_destroy_write_struct(&write_ptr, &write_info_ptr);
        return false;
    }
    png_init_io(write_ptr, fpout);
    png_set_compression_level(write_ptr, Z_BEST_COMPRESSION);
    if (have_IHDR)
        png_set_IHDR(write_ptr, write_info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    write_npTc_chunk(write_ptr, write_info_ptr);
    png_write_info(write_ptr, write_info_ptr);

    png_write_image(write_ptr, rows_buf);
    //png_write_rows(write_ptr, rows_buf, height);
    png_write_end(write_ptr, write_info_ptr);
    png_destroy_write_struct(&write_ptr, &write_info_ptr);
    fclose(fpout);
    return true;
}

cv::Mat PngImage::get_Mat_from_png(){
    if (!have_image)
        return cv::Mat();
    cv::Mat mat(height, width, CV_8UC4);
    for (int i = 0; i < mat.rows; ++i) {
        for (int j = 0; j < mat.cols; ++j) {
            cv::Vec4b &rgba = mat.at<cv::Vec4b>(i, j);
            rgba[0] = rows_buf[i][j * 4 + 2];//B
            rgba[1] = rows_buf[i][j * 4 + 1];//G
            rgba[2] = rows_buf[i][j * 4];//R
            rgba[3] = rows_buf[i][j * 4 + 3];//A
        }
    }
    return mat;
}

bool PngImage::set_npTc_info(const cv::Vec4i &gird, const cv::Vec4i &padding){
    if (!have_image)
        return false;
    npTc.numXDivs = 2;
    npTc.numYDivs = 2;
    npTc.numColors = 9;
    npTc.xDivsOffset = sizeof(char) * 4 + sizeof(unsigned int) * 3 + sizeof(int) * 4;
    npTc.yDivsOffset = sizeof(char) * 4 + sizeof(unsigned int) * 3 + sizeof(int) * (4 + npTc.numXDivs);
    npTc.padding_left = padding[0] < width / 2 ? padding[0] : gird[0];
    npTc.padding_top = padding[1] < height / 2 ? padding[1] : gird[1];
    npTc.padding_right = padding[2] < width / 2 ? padding[2] : gird[2];
    npTc.padding_bottom = padding[3] < height / 2 ? padding[3] : gird[3];
    npTc.XDivs[0] = gird[0];//left
    npTc.XDivs[1] = width - gird[2];//right
    npTc.YDivs[0] = gird[1];//top
    npTc.YDivs[1] = height - gird[3];//bottom
    npTc.colorsOffset = sizeof(char) * 4 + sizeof(unsigned int) * 3 + sizeof(int) * (4 + npTc.numXDivs + npTc.numYDivs);

    unsigned int top = 0, left, right, bottom, colorIndex = 0;
    for (int j = (npTc.YDivs[0] == 0 ? 1 : 0); j <= npTc.numYDivs && top < height; j++) {
        if (j == npTc.numYDivs) {
            bottom = height;
        }
        else {
            bottom = npTc.YDivs[j];
        }
        left = 0;
        for (int i = npTc.XDivs[0] == 0 ? 1 : 0; i <= npTc.numXDivs && left < width; i++) {
            if (i == npTc.numXDivs) {
                right = width;
            }
            else {
                right = npTc.XDivs[i];
            }
            npTc.colors[colorIndex++] = get_nine_patch_color(rows_buf, left, top, right - 1, bottom - 1);
            left = right;
        }
        top = bottom;
    }
    have_npTc = true;
    return true;
}

bool PngImage::write_npTc_chunk(png_structp &write_ptr, png_infop &write_info_ptr){
    if (have_npTc){
        png_unknown_chunk chunks[1];
        strcpy((png_charp)chunks[0].name, "npTc");
        char *buf = (char*)malloc(npTc.get_npTc_size());
        npTc.serialize(buf);
        chunks[0].data = (png_bytep)buf;
        chunks[0].size = sizeof(npTc);
        chunks[0].location = PNG_HAVE_PLTE;
        png_set_unknown_chunks(write_ptr, write_info_ptr, chunks, 1);
        png_set_unknown_chunk_location(write_ptr, write_info_ptr, 0, chunks[0].location);
        free(buf);
        return true;
    }
    return false;
}

bool PngImage::isUnComplied9Png(){
    if (!have_image)
        png_type = PNG_IMAGE_TYPE_NOT_EXIST;
    if (png_type == PNG_IMAGE_TYPE_NORMAL){
        int blackCount = 0;
        bool is9Png = true;
        for (unsigned int i = 0; i < width; i++){
            if (rows_buf[0][i * 4] == 0 &&
                rows_buf[0][i * 4 + 1] == 0 &&
                rows_buf[0][i * 4 + 2] == 0 &&
                rows_buf[0][i * 4 + 3] == 0xFF)
                blackCount++;
            else if (rows_buf[0][i * 4 + 3] == 0x00);
            else{
                is9Png = false;
                break;
            }
        }
        if (blackCount == 0)
            is9Png = false;
        if (is9Png){
            for (unsigned int i = 0; i < width; i++){
                if (rows_buf[height - 1][i * 4] == 0 &&
                    rows_buf[height - 1][i * 4 + 1] == 0 &&
                    rows_buf[height - 1][i * 4 + 2] == 0 &&
                    rows_buf[height - 1][i * 4 + 3] == 0xFF);
                else if (rows_buf[height - 1][i * 4 + 3] == 0x00);
                else{
                    is9Png = false;
                    break;
                }
            }
        }
        if (is9Png){
            blackCount = 0;
            for (unsigned int i = 0; i < height; i++){
                if (rows_buf[i][0] == 0 &&
                    rows_buf[i][1] == 0 &&
                    rows_buf[i][2] == 0 &&
                    rows_buf[i][3] == 0xFF)
                    blackCount++;
                else if (rows_buf[i][3] == 0x00);
                else{
                    is9Png = false;
                    break;
                }
            }
            if (blackCount == 0)
                is9Png = false;
        }
        if (is9Png){
            for (unsigned int i = 0; i < height; i++){
                if (rows_buf[i][(width - 1) * 4 + 0] == 0 &&
                    rows_buf[i][(width - 1) * 4 + 1] == 0 &&
                    rows_buf[i][(width - 1) * 4 + 2] == 0 &&
                    rows_buf[i][(width - 1) * 4 + 3] == 0xFF);
                else if (rows_buf[i][(width - 1) * 4 + 3] == 0x00);
                else{
                    is9Png = false;
                    break;
                }
            }
        }
        if (is9Png)
            png_type = PNG_IMAGE_TYPE_UNCOMPLIED_9PNG;
    }
    if (png_type == PNG_IMAGE_TYPE_UNCOMPLIED_9PNG)
        return true;
    else
        return false;
}

unsigned int PngImage::get_nine_patch_color(png_bytepp rows, int left, int top, int right, int bottom){
    png_bytep color = rows[top] + left * 4;
    if (left > right || top > bottom) {
        return 0x00000000;//Transparent
    }
    while (top <= bottom) {
        for (int i = left; i <= right; i++) {
            png_bytep p = rows[top] + i * 4;
            if (color[3] == 0) {
                if (p[3] != 0) {
                    return 0x00000001;//no color
                }
            }
            else if (p[0] != color[0] || p[1] != color[1]
                || p[2] != color[2] || p[3] != color[3]) {
                return 0x00000001;
            }
        }
        top++;
    }
    if (color[3] == 0) {
        return 0x00000001;
    }
    return (color[3] << 24) | (color[0] << 16) | (color[1] << 8) | color[2];
}

void PngImage::release(){
    if (rows_buf != NULL){
        for (size_t y = 0; y < height; y++){
            free(rows_buf[y]);
        }
        free(rows_buf);
    }
    rows_buf = NULL;
    have_image = false;
    have_npTc = false;
    memset(&npTc, 0, sizeof(npTc_block));
}

PngImage::PNG_IMAGE_TYPE PngImage::getPngType(){
    if (!have_image)
        return PNG_IMAGE_TYPE_NOT_EXIST;
    return png_type;
}

void PngImage::swap_columns(int col1, int col2){
    int y;
    for (y = 0; y < height; y++) {
        png_byte *tmp = (png_byte*)malloc(bytes_per_pixel*sizeof(png_byte));
        memcpy(tmp, &rows_buf[y][col1], bytes_per_pixel);
        memcpy(&rows_buf[y][col1], &rows_buf[y][col2], bytes_per_pixel);
        memcpy(&rows_buf[y][col2], tmp, bytes_per_pixel);
        free(tmp);
    }
}
void PngImage::add_borders(){
    int x, y;

    // extend width
    width += 2;
    for (y = 0; y < height; y++) {
        rows_buf[y] = (png_bytep)realloc(rows_buf[y], sizeof(png_byte)*width*bytes_per_pixel);
        memset(&rows_buf[y][(width - 1)*bytes_per_pixel], 0, bytes_per_pixel);
        memset(&rows_buf[y][(width - 2)*bytes_per_pixel], 0, bytes_per_pixel);
    }
    // center vertically
    for (x = (width - 1)*bytes_per_pixel; x > 0; x -= bytes_per_pixel)
        swap_columns(x, x - bytes_per_pixel);
    // extend height
    height += 2;
    rows_buf = (png_bytep*)realloc(rows_buf, sizeof(png_bytep) * height);
    rows_buf[height - 2] = (png_bytep)malloc(sizeof(png_byte)*width*bytes_per_pixel);
    rows_buf[height - 1] = (png_bytep)malloc(sizeof(png_byte)*width*bytes_per_pixel);
    memset(&rows_buf[height - 2][0], 0, width*bytes_per_pixel);
    memset(&rows_buf[height - 1][0], 0, width*bytes_per_pixel);
    // center horizontally
    for (y = height - 2; y > 0; y--) {
        png_bytep tmp = rows_buf[y];
        rows_buf[y] = rows_buf[y - 1];
        rows_buf[y - 1] = tmp;
    }
}
void PngImage::add_patches(npTc_block * np_block){
    int i, j;
    // XDivs
    for (i = 0; i < np_block->numXDivs; i += 2)
        for (j = np_block->XDivs[i]; j < np_block->XDivs[i + 1]; j++)    {
            png_bytep pix = &(rows_buf[0][(j + 1)*bytes_per_pixel]);
            pix[bytes_per_pixel - 1] = 255;
        }
    // YDivs
    for (i = 0; i < np_block->numYDivs; i += 2)
        for (j = np_block->YDivs[i]; j < np_block->YDivs[i + 1]; j++)    {
            png_bytep pix = &(rows_buf[(j + 1)][0]);
            pix[bytes_per_pixel - 1] = 255;
        }
}

void PngImage::add_paddings(npTc_block * np_block){
    int i;
    //  padding left / right
    for (i = (np_block->padding_left + 1)*bytes_per_pixel; i < (width - np_block->padding_right - 1)*bytes_per_pixel; i += bytes_per_pixel) {
        png_bytep pix = &(rows_buf[height - 1][i]);
        pix[bytes_per_pixel - 1] = 255;
    }
    //  padding bottom / top
    for (i = np_block->padding_top + 1; i < (height - np_block->padding_bottom - 1); i++) {
        png_bytep pix = &(rows_buf[i][(width - 1)*bytes_per_pixel]);
        pix[bytes_per_pixel - 1] = 255;
    }
}

bool PngImage::set_de9patch(){
    if (!have_image)
        return false;
    if (!have_npTc)
        return false;
    add_borders();
    add_patches(&npTc);
    add_paddings(&npTc);
    have_npTc = false;
    return true;
}