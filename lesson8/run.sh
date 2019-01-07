#!/bin/bash
g++ test8.cpp -g \
    -I/home/caffe/Halide/Halide/include \
    -I/home/caffe/Halide/Halide/tools \
    -I/home/caffe/Halide/Halide/tutorial \
    -I/usr/include/libpng12 \
    -L/home/caffe/Halide/Halide/build/lib \
    -L/home/caffe/Halide/Halide/build/bin \
    -L/usr/lib/x86_64-linux-gnu \
    -L/usr/local/lib/ \
    -lHalide  \
    -lpng12  \
    -ljpeg -lpthread -ldl -o test8 -std=c++11 \
    && ./test8
