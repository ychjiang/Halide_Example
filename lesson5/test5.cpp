/*************************************************************************
	> File Name: test5.cpp
	> Author: 
	> Mail: 
	> Created Time: Wed 26 Dec 2018 08:52:14 AM UTC
 ************************************************************************/

#include<iostream>
#include "Halide.h"
#include <algorithm>
using namespace Halide;

int main(){
    Var x("x"),y("y");
    
    //Method 1 Row Major
    {
        Func gradient("gradient");
        //gradient(x,y) = print(x)+y;//output value of y
        gradient(x,y) = x+y;
        gradient.trace_stores();//output value of x+y each element
        std::cout<<"Evaluate gradient row major\n"<<std::endl;
        Buffer<int> output = gradient.realize(4,4);
        gradient.print_loop_nest(); //print out loop schedule 
    }

    //Method 2 Col Major 
    {
        Func gradient("gradient");
        //Change Here
        gradient(x,y) = x+y;
        gradient.reorder(y,x);
        // Equal to
        //gradient1(y,x) = x+y;
        gradient.trace_stores();//output value of x+y each element
        std::cout<<"Evaluate gradient row major\n"<<std::endl;
        Buffer<int> output = gradient.realize(4,4);
        gradient.print_loop_nest(); //print out loop schedule 
    }

    //Method 3 Split Row and Col 
    {
        Func gradient("gradient");
        gradient(x,y) = x+y;
        gradient.trace_stores();
        Var x_outer,x_inner;
        gradient.split(x,x_outer,x_inner,2);//Split x in to x_out and x_inner  x_inner is [0,2)
        Buffer<int> output = gradient.realize(4,4);
        gradient.print_loop_nest(); 
    }

    //Method 4 Evaluate in Tiles
    {
        Func gradient("gradient Tiles");
        gradient(x,y) = x+y;
        gradient.trace_stores();

        Var x_outer,x_inner,y_outer,y_inner;
        gradient.split(x,x_outer,x_inner,2);
        gradient.split(y,y_outer,y_inner,2);
        gradient.reorder(x_inner,y_inner,x_outer,y_outer);//default is for y for x 
        //gradient.reorder(y_inner,x_inner,y_outer,x_outer);
        Buffer<int> output = gradient.realize(4,4);
        gradient.print_loop_nest();

    }
    //Method 5 Vectorize 
    {
        Func gradient("gradient Vectorize");
        gradient(x,y) = x+y;
        gradient.trace_stores();

        Var x_outer,x_inner;
        gradient.split(x,x_outer,x_inner,2);
        gradient.vectorize(x_inner); // Vectorize
        Buffer<int> output = gradient.realize(4,4);
        gradient.print_loop_nest();//Calculate in each Vector
    }

    //Method 6 Vectorize 7X2
    {
        Func gradient("gradient Vectorize 7X2");
        gradient(x+y) = x+y;
        gradient.trace_stores();
        Var x_outer,x_inner;
        gradient.split(x,x_outer,x_inner,3);
        Buffer<int> output = gradient.realize(7,2);
        gradient.print_loop_nest();

    }
}
