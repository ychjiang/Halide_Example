/*************************************************************************
	> File Name: test2.cpp
	> Author: 
	> Mail: 
	> Created Time: Tue 25 Dec 2018 12:24:22 PM UTC
 ************************************************************************/

#include<iostream>
#include "Halide.h"
#include "/home/caffe/Halide/Halide/tools/halide_image_io.h"
using namespace Halide::Tools;

int main(){
    Halide::Buffer<uint8_t> input = load_image("./input.jpg");
    Halide::Func brighter;
    Halide::Var x,y,c;
    Halide::Expr value = input(x,y,c);
    value =value*1.5f; //整体提升像素值 亮化处理
    value = Halide::min(value,255.0f);
    value = Halide::cast<uint8_t>(value);
    brighter(x,y,c) = value;
    Halide::Buffer<uint8_t> output = brighter.realize(input.width(),\
                                                      input.height(),input.channels());
    save_image(output,"output.jpg");
}

