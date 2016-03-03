# image_compress
PROJECT(IMAGE_COMPRESS C CXX)

# min version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# define compile std
SET(CMAKE_CXX_FLAGS "-std=c++11")
SET(CMAKE_C_FLAGS "-std=c11")
IF(UNIX AND NOT APPLE)
    SET(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
ENDIF()

# SET(BOOST_ROOT "/usr/local/lib")

# set opencv config
SET(OpenCV_STATIC ON)

# find opencv package
FIND_PACKAGE(OpenCV REQUIRED)

# set boost config
SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_MULTITHREAD ON)

# find boost package
FIND_PACKAGE(Boost REQUIRED COMPONENTS filesystem program_options system regex)

# set bin dir
SET(EXECUTABLE_OUTPUT_PATH ${IMAGE_COMPRESS_SOURCE_DIR}/bin)

# set lib dir
SET(LIBRARY_OUTPUT_PATH ${IMAGE_COMPRESS_SOURCE_DIR}/lib)

# set include dirs
INCLUDE_DIRECTORIES(
/usr/local/include/
/usr/local/lib/include/
${IMAGE_COMPRESS_SOURCE_DIR}/
${IMAGE_COMPRESS_SOURCE_DIR}/src/
${IMAGE_COMPRESS_SOURCE_DIR}/src/zlib/
${IMAGE_COMPRESS_SOURCE_DIR}/src/libpng/
${IMAGE_COMPRESS_SOURCE_DIR}/src/apngasm/
${IMAGE_COMPRESS_SOURCE_DIR}/src/zopfli/
${IMAGE_COMPRESS_SOURCE_DIR}/src/mozjpeg/
)

#MESSAGE(STATUS "{Boost_INCLUDE_DIRS}: " ${Boost_INCLUDE_DIRS})

# set lib dirs
LINK_DIRECTORIES(/usr/local/lib/ ${Boost_LIBRARY_DIRS}/)

# add sub dir
ADD_SUBDIRECTORY(src)