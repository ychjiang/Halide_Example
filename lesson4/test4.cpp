/*************************************************************************
	> File Name: test4.cpp
	> Author: 
	> Mail: 
	> Created Time: Wed 26 Dec 2018 07:29:51 AM UTC
 ************************************************************************/

#include<iostream>
#include "Halide.h"
/*
 * Default data 
 * x=[0 1 2, 0 1 2, 0 1 2]
 * y=trans(x)
*/
int main(){
    Halide::Var x("x"),y("y");
    {
        // step 1 add serial 
        Halide::Func gradient("gradient");
        gradient(x,y) = x+y;
        gradient.trace_stores();
        std::cout<<"Evaluate gradient\n"<<std::endl;
        Halide::Buffer<int> output = gradient.realize(3,3);
        
        // step 2 add parallel with x and y 
        Halide::Func parallel_gradient("parallel_gradient");
        parallel_gradient(x,y) = x+y;
        parallel_gradient.trace_stores();
        parallel_gradient.parallel(x);
        parallel_gradient.parallel(y);
        parallel_gradient.realize(3,3);
    }
    // trace_stores() can only get the value of a function 
    // if you want to see subparameters,you can use print(param)
    {
        Halide::Func f;
        f(x,y) = Halide::sin(x)+Halide::cos(y);
        Halide::Func g;
        g(x,y) = Halide::sin(x) + Halide::print(Halide::cos(y)); // value of cos(y)
        g(x,y) = Halide::sin(x) + Halide::cos(Halide::print(y)); // value of y 
        //g(x,y) = Halide::sin(x) + Halide::cos(y);
        //g.trace_stores();
        g.realize(3,3);
    }
    std::cout<<"Success\n"<<std::endl;
    return 0;
}
