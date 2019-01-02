/*************************************************************************
	> File Name: test1.c
	> Author: 
	> Mail: 
	> Created Time: Mon 24 Dec 2018 01:07:07 PM UTC
 ************************************************************************/

#include<stdio.h>
#include "Halide.h"
int main(void)
{
    printf("Hello Halide\n");
    Halide::Func gradient;
    Halide::Var x,y;
    Halide::Expr e = x+y;
    gradient(x,y) = e;
    Halide::Buffer<int32_t> output = gradient.realize(80,60);
    for(int j = 0; j<output.height(); j++){
        for(int i = 0; i<output.width(); i++){
            if(output(i,j) != i+j){
                printf("Calculate Error in %d %d and value is %d \n",i,j,output(i,j));
                return -1;
            }
        }
    }
    printf("Calculate Success\n");
    return 0;
}
