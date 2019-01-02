/*************************************************************************
	> File Name: test3.cpp
	> Author: 
	> Mail: 
	> Created Time: Tue 25 Dec 2018 01:57:13 PM UTC
 ************************************************************************/

#include<iostream>
#include "Halide.h"
int main(){
    Halide::Func gradient("gradient");
    Halide::Var x("x");
    Halide::Var y("y");
    gradient(x,y) = x+y;
    Halide::Buffer<int> output = gradient.realize(800,800);
    gradient.compile_to_lowered_stmt("gradient.html",{},Halide::HTML);
    std::cout<<"Success \n"<<std::endl;
}

