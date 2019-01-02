/*************************************************************************
	> File Name: test5.cpp
	> Author: 
	> Mail: 
	> Created Time: Wed 26 Dec 2018 08:52:14 AM UTC
 ************************************************************************/

#include<iostream>
#include "Halide.h"
#include "halide_image_io.h" 
#include <time.h> 
using namespace Halide;
using namespace Halide::Tools;
int main(){
    Var x("x"),y("y"),c("c");

    //Method 1 image size decrease
    {
        Buffer<uint8_t> input = load_image("./rgb.jpg");
        Func input_8("input_8");
        Func input_16("input_16");
        input_8(x,y,c) = input(x,y,c);  
        input_16(x,y,c) = cast<uint16_t>(input(x,y,c)); //cast int8 to int16 
        clock_t start1 = clock();
        input_8.realize(input);
        clock_t end1 = clock();
        std::cout<<"Use time1 "<<(double)(end1-start1)/CLOCKS_PER_SEC<<"s"<<std::endl;
        // we use this way to accelerate image peocess.
        //
        Func blur_x("blur_x");
        blur_x(x,y,c) = ((input_16(x-1,y,c))+2*input_16(x,y,c)+input_16(x+1,y,c))/4; // blur it horizontally
        Func blur_y("blur_y");
        blur_y(x,y,c) = ((blur_x(x,y-1,c))+2*blur_x(x,y,c)+blur_x(x,y+1,c))/4; // blur it vertically
        Func output("output");
        output(x,y,c) = cast<uint8_t>(blur_y(x,y,c));
        Buffer<uint8_t> result(input.width()-2,input.height()-2,3);
        result.set_min(1,1);
        clock_t start2 = clock();
        output.realize(result);
        clock_t end2 = clock();
        std::cout<<"Use time2 "<<(double)(end2-start2)/CLOCKS_PER_SEC<<"s"<<std::endl;
        save_image(result,"result_output.jpg");
    }

    //Method 2 image size same 
    {
        Buffer<uint8_t>input = load_image("./rgb.jpg");
        Func clamped("clamped");
        Expr clamped_x = clamp(x,0,input.width()-1);
        Expr clamped_y = clamp(y,0,input.height()-1);
        clamped(x,y,c) = input(clamped_x,clamped_y,c);
        Func input_16("input_16");
        input_16(x,y,c) = cast<uint16_t>(clamped(x,y,c));
        Func blur_x("blur_x");
        blur_x(x,y,c)= (input_16(x-1,y,c)+2*input_16(x,y,c)+input_16(x+1,y,c))/4;
        Func blur_y("blur_y");
        blur_y(x,y,c) = (blur_x(x-1,y,c)+2*blur_x(x,y,c)+blur_x(x+1,y,c))/4;
        Func output("output");
        output(x,y,c) = cast<uint8_t>(blur_y(x,y,c));
        Buffer<uint8_t> result = output.realize(input.width(),input.height(),3);
        save_image(result,"new_result.jpg");
    }

}
