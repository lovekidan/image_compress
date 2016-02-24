# image_compress
PROJECT(IMAGE_COMPRESS C CXX)

# min version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# set bin dir
set(EXECUTABLE_OUTPUT_PATH ${IMAGE_COMPRESS_SOURCE_DIR}/bin)

# set lib dir
set(LIBRARY_OUTPUT_PATH ${IMAGE_COMPRESS_SOURCE_DIR}/lib)

# add include dirs
include_directories(
/usr/local/include/opencv2
${IMAGE_COMPRESS_SOURCE_DIR}/src/
${IMAGE_COMPRESS_SOURCE_DIR}/include/
)

# add sub dir
ADD_SUBDIRECTORY(src)

# find opencv
FIND_PACKAGE(OpenCV REQUIRED)