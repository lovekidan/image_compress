# mozjpeg
IF(MSVC AND CMAKE_CL_64)
  SET(SIMD_X86_64 1)
ELSEIF(CMAKE_SIZEOF_VOID_P MATCHES 8)
  SET(SIMD_X86_64 1)
ENDIF()

SET(SRC_MOZJPEG mozjpeg/)
SET(JPEG_SOURCES jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jcext.c
    jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c
    jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c jdatadst.c
    jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdinput.c jdmainct.c
    jdmarker.c jdmaster.c jdmerge.c jdphuff.c jdpostct.c jdsample.c jdtrans.c
    jerror.c jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c
    jidctred.c jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c)


SET(JPEG_SOURCES ${JPEG_SOURCES} jaricom.c)
SET(JPEG_SOURCES ${JPEG_SOURCES} jcarith.c)
SET(JPEG_SOURCES ${JPEG_SOURCES} jdarith.c)

# simd support
SET(JPEG_SOURCES ${JPEG_SOURCES} jsimd_none.c)
#ADD_DEFINITIONS(-DWITH_SIMD)
#ADD_SUBDIRECTORY(${SRC_MOZJPEG}simd)
#IF(SIMD_X86_64)
#    SET(JPEG_SOURCES ${JPEG_SOURCES} simd/jsimd_x86_64.c)
#ELSE()
#    SET(JPEG_SOURCES ${JPEG_SOURCES} simd/jsimd_i386.c)
#ENDIF()

# turbojpeg lib
SET(JPEG_SOURCES ${JPEG_SOURCES} turbojpeg.c transupp.c jdatadst-tj.c jdatasrc-tj.c)

# This tells CMake that the "source" files haven't been generated yet
#SET_SOURCE_FILES_PROPERTIES(${SIMD_OBJS} PROPERTIES GENERATED 1)

# cjpeg 
SET(JPEG_SOURCES ${JPEG_SOURCES} cdjpeg.c rdjpeg.c rdswitch.c)


SET(JPEG_SRC_FULLPATH "")
FOREACH(SRC_FILE ${JPEG_SOURCES})
    SET(JPEG_SRC_FULLPATH ${JPEG_SRC_FULLPATH} ${SRC_MOZJPEG}${SRC_FILE})
ENDFOREACH(SRC_FILE ${JPEG_SOURCES})
#MESSAGE(STATUS "{JPEG_SRC_FULLPATH}: ${JPEG_SRC_FULLPATH}")
#MESSAGE(STATUS "{SIMD_OBJS}: ${SIMD_OBJS}")


# aux source file as vars
AUX_SOURCE_DIRECTORY(. SRC_ROOT)
AUX_SOURCE_DIRECTORY(icutil/ SRC_ICUTIL)
AUX_SOURCE_DIRECTORY(pngimage/ SRC_PNGIMAGE)
AUX_SOURCE_DIRECTORY(zlib/ SRC_ZLIB)
AUX_SOURCE_DIRECTORY(libpng/ SRC_LIBPNG)
AUX_SOURCE_DIRECTORY(pngquant/ SRC_PNGQUANT)
AUX_SOURCE_DIRECTORY(pngquant/lib/ SRC_PNGQUANT_LIB)
AUX_SOURCE_DIRECTORY(zopfli/ SRC_ZOPFLI)
AUX_SOURCE_DIRECTORY(zopflipng/ SRC_ZOPFLIPNG)
AUX_SOURCE_DIRECTORY(zopflipng/lodepng/ SRC_PNGQUANT_LODEPNG)
AUX_SOURCE_DIRECTORY(apngasm/ SRC_APNGASM)
AUX_SOURCE_DIRECTORY(apngasm/cli/ SRC_APNGASM_CLI)
AUX_SOURCE_DIRECTORY(apngasm/lib/ SRC_APNGASM_LIB)
AUX_SOURCE_DIRECTORY(apngasm/lib/listener/ SRC_APNGASM_LIB_LISTENER)
AUX_SOURCE_DIRECTORY(apngasm/lib/spec/ SRC_APNGASM_LIB_SPEC)
AUX_SOURCE_DIRECTORY(apngasm/lib/spec/priv/ SRC_APNGASM_LIB_SPEC_PRIV)
AUX_SOURCE_DIRECTORY(${SRC_MOZJPEG}wrapper/ SRC_CJPEG_WRAPPER)
# AUX_SOURCE_DIRECTORY(ninepatch/ SRC_NINEPATCH)

# add executable
ADD_EXECUTABLE(image_compress ${SRC_ROOT} ${SRC_ZLIB} ${SRC_LIBPNG} 
    ${SRC_PNGIMAGE} ${SRC_ICUTIL} ${SRC_PNGQUANT} ${SRC_PNGQUANT_LIB} 
    ${SRC_ZOPFLI} ${SRC_ZOPFLIPNG} ${SRC_PNGQUANT_LODEPNG} ${SRC_APNGASM}
    ${SRC_APNGASM_LIB} ${SRC_APNGASM_CLI} ${SRC_APNGASM_LIB} ${SRC_APNGASM_CLI} 
    ${SRC_APNGASM_LIB_LISTENER} ${SRC_APNGASM_LIB_SPEC} ${SRC_APNGASM_LIB_SPEC_PRIV} 
    ${SRC_NINEPATCH} ${SRC_CJPEG_WRAPPER} ${JPEG_SRC_FULLPATH} ${SRC_NINEPATCH})

MESSAGE(STATUS,"CV="${OpenCV_LIBS})

MESSAGE(STATUS,"BST="${Boost_LIBRARIES})

# link libs
TARGET_LINK_LIBRARIES(image_compress ${OpenCV_LIBS} ${Boost_LIBRARIES})
#MESSAGE(STATUS "{SRC_CJPEG_WRAPPER}: ${SRC_CJPEG_WRAPPER}")
#MESSAGE(STATUS "{Boost_LIBRARIES}: ${Boost_LIBRARIES}")

#ADD_SUBDIRECTORY(mozjpeg)
