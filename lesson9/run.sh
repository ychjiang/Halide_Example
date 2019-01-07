#!/bin/bash
g++ test9.cpp -g \
    -I/home/caffe/Halide/Halide/include \
    -I/home/caffe/Halide/Halide/tools \
    -L/home/caffe/Halide/Halide/build/lib \
    -L/home/caffe/Halide/Halide/build/bin \
    -L/usr/lib/x86_64-linux-gnu \
    -L/usr/local/lib/ \
    -lHalide  \
    -ljpeg -lpthread -ldl -o test9 -std=c++11 \
    && ./test9
