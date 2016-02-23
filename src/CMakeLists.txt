AUX_SOURCE_DIRECTORY(. SRC_ROOT_DIR)
AUX_SOURCE_DIRECTORY(/pngimage SRC_PNGIMAGE_DIR)
ADD_EXECUTABLE(image_compress ${SRC_ROOT_DIR} ${SRC_PNGIMAGE_DIR})
#target_link_libraries(linux_awesome_proj linux_awesome_proj_lib ${LIBRARY_OUTPUT_PATH}/liboutside.a)