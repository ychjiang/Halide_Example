#!/bin/bash
g++ test7.cpp -g \
    -I/home/caffe/Halide/Halide/include \
    -I/home/caffe/Halide/Halide/tools \
    -L/home/caffe/Halide/Halide/build/lib \
    -L/home/caffe/Halide/Halide/build/bin \
    -L/usr/lib/x86_64-linux-gnu \
    -L/usr/local/lib/ \
    -lHalide  \
    -ljpeg -lpthread -ldl -o test7 -std=c++11 \
    && ./test7
