#!/bin/bash
g++ test3.cpp -pg \
    -I/home/caffe/Halide/Halide/include \
    -I/home/caffe/Halide/Halide/tools \
    -L/home/caffe/Halide/Halide/build/lib \
    -L/home/caffe/Halide/Halide/build/bin \
    -L/usr/lib/x86_64-linux-gnu \
    -L/usr/local/lib/ \
    -lHalide  \
    -lpthread -ldl -o test3 -std=c++11 \
    && ./test3
    #&& export HL_DEBUG_CODEGEN=2 && ./test3
