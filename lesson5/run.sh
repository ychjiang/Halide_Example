#!/bin/bash
g++ test5.cpp -g \
    -I/home/caffe/Halide/Halide/include \
    -I/home/caffe/Halide/Halide/tools \
    -L/home/caffe/Halide/Halide/build/lib \
    -L/home/caffe/Halide/Halide/build/bin \
    -L/usr/local/lib/ \
    -lHalide  \
    -lpthread -ldl -o test5 -std=c++11 \
    && ./test5

