#!/bin/bash
g++ test1.cpp -pg -L/home/caffe/Halide/Halide/build/lib -lHalide -lpthread -ldl -o test1 --std=c++11 && ./test1
