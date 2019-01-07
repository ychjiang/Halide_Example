// Halide tutorial lesson 11: Cross-compilation
// //跨平台编译

// This lesson demonstrates how to use Halide as a cross-compiler to
// generate code for any platform from any platform.
//这一节将展示如何利用Halide实现跨平台编译
//为不同的平台生成代码

#include "Halide.h"
#include <stdio.h>
using namespace Halide;

int main(int argc, char **argv) {

    // We'll define the simple one-stage pipeline that we used in lesson 10.
    Func brighter;
    Var x, y;

    // Declare the arguments.
    Param<uint8_t> offset;
    ImageParam input(type_of<uint8_t>(), 2);
    std::vector<Argument> args(2);
    args[0] = input;
    args[1] = offset;

    // Define the Func.
    brighter(x, y) = input(x, y) + offset;

    // Schedule it.设置运算策略
    brighter.vectorize(x, 16).parallel(y);

    // The following line is what we did in lesson 10. It compiles an
    // object file suitable for the system that you're running this
    // program on.  For example, if you compile and run this file on
    // 64-bit linux on an x86 cpu with sse4.1, then the generated code
    // will be suitable for 64-bit linux on x86 with sse4.1.
    // 接下来的代码编译产生适合你当前运行环境的目标文件。
    // 例如你当前编译和运行该文件在64位的linux x86 CPU 系统上
    // 改代码将产生适合该环境的代码
    brighter.compile_to_file("lesson_11_host", args, "brighter");

    // We can also compile object files suitable for other cpus and
    // operating systems. You do this with an optional third argument
    // to compile_to_file which specifies the target to compile for.
    // 也可以编译成适合其他CPU型号和操作系统的代码
    // 只要在compile_to_file中指定目标和编译指向就可以了 

    // Let's use this to compile a 32-bit arm android version of this code:
    // 我们来看一下如何编译成arm下安卓32位的代码版本

    Target target;
    target.os = Target::Android; // The operating system
    target.arch = Target::ARM;   // The CPU architecture
    target.bits = 32;            // The bit-width of the architecture
    std::vector<Target::Feature> arm_features; // A list of features to set
    target.set_features(arm_features);//设置编译环境

    // We then pass the target as the last argument to compile_to_file.
    // 将设定好的target在compile_to_file中指定出来
    brighter.compile_to_file("lesson_11_arm_32_android", args, "brighter", target);

    // And now a Windows object file for 64-bit x86 with AVX and SSE 4.1:
    // 相同的，编译一个windows64位的目标文件
    target.os = Target::Windows;
    target.arch = Target::X86;
    target.bits = 64;
    std::vector<Target::Feature> x86_features;
    x86_features.push_back(Target::AVX);
    x86_features.push_back(Target::SSE41);
    target.set_features(x86_features);
    brighter.compile_to_file("lesson_11_x86_64_windows", args, "brighter", target);

    // And finally an iOS mach-o object file for one of Apple's 32-bit
    // ARM processors - the A6. It's used in the iPhone 5. The A6 uses
    // a slightly modified ARM architecture called ARMv7s. We specify
    // this using the target features field.  Support for Apple's
    // 64-bit ARM processors is very new in llvm, and still somewhat
    // flaky.
    // 接下来是 IOS环境下的代码 
    target.os = Target::IOS;
    target.arch = Target::ARM;
    target.bits = 32;
    std::vector<Target::Feature> armv7s_features;
    armv7s_features.push_back(Target::ARMv7s);
    target.set_features(armv7s_features);
    brighter.compile_to_file("lesson_11_arm_32_ios", args, "brighter", target);


    // Now let's check these files are what they claim, by examining
    // their first few bytes.
    // 接下来检测这些生成的代码是否符合要求
    // 主要的方法是测试他们的前几个字节

    // 32-arm android object files start with the magic bytes:
    // 32位的安卓arm 目标文件是由magic字节开始的
    uint8_t arm_32_android_magic[] = {0x7f, 'E', 'L', 'F', // ELF format
                                      1,       // 32-bit
                                      1,       // 2's complement little-endian
                                      1};      // Current version of elf

    FILE *f = fopen("lesson_11_arm_32_android.o", "rb");//打开arm 32的安卓文件
    uint8_t header[32];
    if (!f || fread(header, 32, 1, f) != 1) {
        printf("Object file not generated\n");
        return -1;
    }
    fclose(f);
    //比较文件 
    if (memcmp(header, arm_32_android_magic, sizeof(arm_32_android_magic))) {
        printf("Unexpected header bytes in 32-bit arm object file.\n");
        return -1;
    }

    // 64-bit windows object files start with the magic 16-bit value 0x8664
    // (presumably referring to x86-64)
    // 64位的Windows代码开始的16位是 0x8664
    uint8_t win_64_magic[] = {0x64, 0x86};

    f = fopen("lesson_11_x86_64_windows.obj", "rb");
    if (!f || fread(header, 32, 1, f) != 1) {
        printf("Object file not generated\n");
        return -1;
    }
    fclose(f);

    if (memcmp(header, win_64_magic, sizeof(win_64_magic))) {
        printf("Unexpected header bytes in 64-bit windows object file.\n");
        return -1;
    }

    // 32-bit arm iOS mach-o files start with the following magic bytes:
    //ios相同的情况，可以检测
    uint32_t arm_32_ios_magic[] = {0xfeedface, // Mach-o magic bytes
                                   12,  // CPU type is ARM
                                   11,  // CPU subtype is ARMv7s
                                   1};  // It's a relocatable object file.
    f = fopen("lesson_11_arm_32_ios.o", "rb");
    if (!f || fread(header, 32, 1, f) != 1) {
        printf("Object file not generated\n");
        return -1;
    }
    fclose(f);

    if (memcmp(header, arm_32_ios_magic, sizeof(arm_32_ios_magic))) {
        printf("Unexpected header bytes in 32-bit arm ios object file.\n");
        return -1;
    }

    // It looks like the object files we produced are plausible for
    // those targets. We'll count that as a success for the purposes
    // of this tutorial. For a real application you'd then need to
    // figure out how to integrate Halide into your cross-compilation
    // toolchain. There are several small examples of this in the
    // Halide repository under the apps folder. See HelloAndroid and
    // HelloiOS here:
    // https://github.com/halide/Halide/tree/master/apps/
    // 上面的目标文件看上去貌似是对的（似是而非）
    // 在世界的应用中，你还需要支出如何将Halide集成到跨平台工具链中
    // 在源码中，有具体的几个例子可以使用
    printf("Success!\n");
    return 0;
}
