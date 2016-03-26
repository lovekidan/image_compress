[![Build Status](https://travis-ci.org/chraac/image_compress.svg?branch=master)](https://travis-ci.org/chraac/image_compress)

# Image_compress
This tool intent to compress various type of image, for now it supports png apng and jpeg

# There are 2 Steps before build the project

## step1:config opencv enviroment
### see http://docs.opencv.org/3.0-last-rst/doc/tutorials/introduction/linux_install/linux_install.html
###a) download the opencv-3.0.0 source code for linux;
###b) open the opencv-3.0.0 path and mkdir build;
###c) build and install opencv in your machine by input command "cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local/ -DBUILD_SHARED_LIBS=OFF ..";
###d) set opencv to your system environment;

## step2:config boost environment
###a) download the boost-1.57 source code for linux;
###b) build and install opencv in your machine by "./bootstrap.sh" and "./b2" and "sudo ./b2 install";

