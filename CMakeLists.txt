# image_compress
PROJECT(IMAGE_COMPRESS C CXX)

# min version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# set bin dir
SET(EXECUTABLE_OUTPUT_PATH ${IMAGE_COMPRESS_SOURCE_DIR}/bin)

# set lib dir
SET(LIBRARY_OUTPUT_PATH ${IMAGE_COMPRESS_SOURCE_DIR}/lib)

# set include dirs
INCLUDE_DIRECTORIES(
/usr/local/include/opencv2
${IMAGE_COMPRESS_SOURCE_DIR}/src/
${IMAGE_COMPRESS_SOURCE_DIR}/src/zlib/
${IMAGE_COMPRESS_SOURCE_DIR}/include/
)

# set lib dirs
LINK_DIRECTORIES(/usr/local/lib)

# add sub dir
ADD_SUBDIRECTORY(src)