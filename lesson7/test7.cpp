// Halide tutorial lesson 7:
// Scheduling multi-stage pipelines
#include "Halide.h"
#include <stdio.h>

using namespace Halide;

int main(int argc, char **argv) {

    Var x("x"), y("y");

    // Let's examine various scheduling options for a simple two stage
    // pipeline. We'll start with the default schedule:
    // 我们来测试两种pipeline下不同的策略
    // 方案1： 默认的策略
    // 默认的策略我们称作compute_inline，实际上就是内联，producer根本没出现。
    {
        Func producer("producer_default"), consumer("consumer_default");

        // The first stage will be some simple pointwise math similar
        // to our familiar gradient function. The value at position x,
        // y is the sin of product of x and y.
        producer(x, y) = sin(x * y);

        // Now we'll add a second stage which averages together multiple
        // points in the first stage.
        consumer(x, y) = (producer(x, y) +
                          producer(x, y+1) +
                          producer(x+1, y) +
                          producer(x+1, y+1))/4;

        // We'll turn on tracing for both functions.
        //consumer.trace_stores();
        producer.trace_stores();
        consumer.trace_stores();

        // And evaluate it over a 4x4 box.
        printf("\nEvaluating producer-consumer pipeline with default schedule\n");
        consumer.realize(4, 4);
        //实际上，默认的策略就是将producer在consumer里面替换掉

        // There were no messages about computing values of the
        // producer. This is because the default schedule fully
        // inlines 'producer' into 'consumer'. It is as if we had
        // written the following code instead:

        // consumer(x, y) = (sin(x * y) +
        //                   sin(x * (y + 1)) +
        //                   sin((x + 1) * y) +
        //                   sin((x + 1) * (y + 1))/4);

        // All calls to 'producer' have been replaced with the body of
        // 'producer', with the arguments substituted in for the
        // variables.
        //Halide使用一种inline的默认策略，
        //所谓的inline策略就是consumer一旦需要producer就理解产生这个数据，即用即生成
        // The equivalent C code is:
        // 默认策略等价的C语言代码如下：
        float result[3][3];
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                result[y][x] = (sin(x*y) +
                                sin(x*(y+1)) +
                                sin((x+1)*y) +
                                sin((x+1)*(y+1)))/4;
                printf(" x=%d\t\ty=%d\t\tresult=%f\n",x,y,result[y][x]);
            }
        }
        printf("\n");

        // If we look at the loop nest, the producer doesn't appear
        // at all. It has been inlined into the consumer.
        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");
    }

    // Next we'll examine the next simplest option - computing all
    // values required in the producer before computing any of the
    // consumer. We call this schedule "root".
    // 方案2：我们测试另一种简单的选项，在计算consumer之前
    // 我们首先计算所需要的producer数值，我们将这种策略称作compute_root.
    {
        // Start with the same function definitions:
        Func producer("producer_root"), consumer("consumer_root");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y+1) +
                          producer(x+1, y) +
                          producer(x+1, y+1))/4;

        // Tell Halide to evaluate all of producer before any of consumer.
        producer.compute_root(); //compute root策略

        // Turn on tracing.
        consumer.trace_stores();
        producer.trace_stores();

        // Compile and run.
        printf("\nEvaluating producer.compute_root()\n");
        consumer.realize(4, 4);
         
        // Reading the output we can see that:
        // A) There were stores to producer.
        // B) They all happened before any stores to consumer.

        // See below for a visualization.
        // The producer is on the left and the consumer is on the
        // right. Stores are marked in orange and loads are marked in
        // blue.
        //从输出代码中我们可以看到：
        //A)producer计算并被存储下来
        //B）在consumer计算之前，producer的处理已经发生
         

        // Equivalent C:
        // 等价的C语言代码如下：

        float result[4][4];

        // Allocate some temporary storage for the producer.
        float producer_storage[5][5];

        // Compute the producer.
        for (int y = 0; y < 5; y++) {
            for (int x = 0; x < 5; x++) {
                producer_storage[y][x] = sin(x * y);
            }
        }

        // Compute the consumer. Skip the prints this time.
        printf("Consumer:\n");
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                result[y][x] = (producer_storage[y][x] +
                                producer_storage[y+1][x] +
                                producer_storage[y][x+1] +
                                producer_storage[y+1][x+1])/4;
                printf("x=%d\t\ty=%d\t\tresult=%f\n",x,y,result[y][x]);
            }
        }

        // Note that consumer was evaluated over a 4x4 box, so Halide
        // automatically inferred that producer was needed over a 5x5
        // box. This is the same 'bounds inference' logic we saw in
        // the previous lesson, where it was used to detect and avoid
        // out-of-bounds reads from an input image.
       //可以看到，consumer实际上在一个4X4的块状区域执行，因此Halide自动的
       //推理出producer需要一个5X5的方块，这种边界推理和上一课时见到的一致
       //主要是为了检测和避免一个输入图像的越界
        // If we print the loop nest, we'll see something very
        // similar to the C above.
        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");
    }

    // Let's compare the two approaches above from a performance
    // perspective.
    //对比上述两种方案：
    // Full inlining (the default schedule):
    // - Temporary memory allocated: 0
    // - Loads: 0
    // - Stores: 16
    // - Calls to sin: 64
    //默认的内联方案：
    //临时存储空间的开辟： 0 
    //加载次数： 0次
    //存储次数： 16次
    //调用sin函数次数：64次

    // producer.compute_root():
    // - Temporary memory allocated: 25 floats
    // - Loads: 64
    // - Stores: 41
    // - Calls to sin: 25
    //compute_root方案：
    //临时空间的开辟：25个单元（4+1）X (4+1)
    //加载次数：64次（4*4*4）一次加载4个值，一共加载4*4次
    //存数次数：41次 （25+16）
    //sin调用次数：25次
    // There's a trade-off here. Full inlining used minimal temporary
    // memory and memory bandwidth, but did a whole bunch of redundant
    // expensive math (calling sin). It evaluated most points in
    // 'producer' four times. The second schedule,
    // producer.compute_root(), did the mimimum number of calls to
    // sin, but used more temporary memory and more memory bandwidth.
    //这里是一种平衡，
    //1）采用默认的内联模式使用了最小的临时存储和传输带宽，
    //   但是在每次计算的时候都需要进行sin的计算，带来了大量的冗余
    //2）采用compute_root的方案调用sin的次数最少，但是使用了很多的
    //   临时存数和数据带宽。

    // In any given situation the correct choice can be difficult to
    // make. If you're memory-bandwidth limited, or don't have much
    // memory (e.g. because you're running on an old cell-phone), then
    // it can make sense to do redundant math. On the other hand, sin
    // is expensive, so if you're compute-limited then fewer calls to
    // sin will make your program faster. Adding vectorization or
    // multi-core parallelism tilts the scales in favor of doing
    // redundant work, because firing up multiple cpu cores increases
    // the amount of math you can do per second, but doesn't increase
    // your system memory bandwidth or capacity.
    //在任何的情况下都很难做出做出最好的选择
    //如果你受制于存储带宽或没有足够的内存，例如在老旧的手机上运行代码
    //你可以通过冗余的计算来换取存储的限制。
    //另一方面，sin的计算代价是非常高昂的，如果你受制于算力，那么尽可能的
    //少调用sin将使你的程序变快。
    //加入向量化或者多核并行化来做冗余的工作（计算sin）,因为一旦打开了多核处理
    //可以在一个时刻进行多个计算，而不增加系统的内存。

    // We can make choices in between full inlining and
    // compute_root. Next we'll alternate between computing the
    // producer and consumer on a per-scanline basis:
   // 我们可以在内联法和root法之间做一个折衷方案：
    {
        // Start with the same function definitions:
        Func producer("producer_y"), consumer("consumer_y");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y+1) +
                          producer(x+1, y) +
                          producer(x+1, y+1))/4;

        // Tell Halide to evaluate producer as needed per y coordinate
        // of the consumer:
        // 告诉Halide执行consumer每一个y坐标循环时需要的producer。
        producer.compute_at(consumer, y);//采用这种计算方案

        // This places the code that computes the producer just
        // *inside* the consumer's for loop over y, as in the
        // equivalent C below.
        // 这种compute_at将producer的计算嵌入到consumer的y循环内部

        // Turn on tracing.
        producer.trace_stores();
        consumer.trace_stores();

        // Compile and run.
        printf("\nEvaluating producer.compute_at(consumer, y)\n");
        consumer.realize(4, 4);

        // See below for a visualization.
        // Reading the log or looking at the figure you should see
        // that producer and consumer alternate on a per-scanline
        // basis. Let's look at the equivalent C:

        float result[4][4];

        // There's an outer loop over scanlines of consumer:
        for (int y = 0; y < 4; y++) {

            // Allocate space and compute enough of the producer to
            // satisfy this single scanline of the consumer. This
            // means a 5x2 box of the producer.
            // 开辟和计算足够的producer数量来供应单个consumer循环
            // 这意味着一次只需要计算5*2个producer区域就可以了
            float producer_storage[2][5];
            for (int py = y; py < y + 2; py++) {
                for (int px = 0; px < 5; px++) {
                    producer_storage[py-y][px] = sin(px * py);
                }
            }

            // Compute a scanline of the consumer.
            for (int x = 0; x < 4; x++) {
                result[y][x] = (producer_storage[0][x] +
                                producer_storage[1][x] +
                                producer_storage[0][x+1] +
                                producer_storage[1][x+1])/4;
            }
        }

        // Again, if we print the loop nest, we'll see something very
        // similar to the C above.
        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");

        // The performance characteristics of this strategy are in
        // between inlining and compute root. We still allocate some
        // temporary memory, but less that compute_root, and with
        // better locality (we load from it soon after writing to it,
        // so for larger images, values should still be in cache). We
        // still do some redundant work, but less than full inlining:
        //这种策略是介于inline和root之间的，我们依旧开辟一些临时存储，
        //但是少于root策略下的存储量，同时拥有更好的局部性（我们很快的
        //将计算结果加载进来，当他们被写入之后，对于大型的图片，数值依旧
        //在cache里面），我们也做了一些冗余的工作，但是数量小于inline模式
        
        // producer.compute_at(consumer, y):
        // - Temporary memory allocated: 10 floats
        // - Loads: 64
        // - Stores: 56
        // - Calls to sin: 40
        // - compute_y策略：
        // - 临时内存开辟：10个float单元（5*2）
        // - 加载次数：64次 循环4*4次 每次加载4个producer
        // - 存储次数：56次 一次计算出2*5个producer，总共循环4次 40个
        //     另外还有16个consumer的值
        // - 调用sin的次数：40次
    }

    // We could also say producer.compute_at(consumer, x), but this
    // would be very similar to full inlining (the default
    // schedule). Instead let's distinguish between the loop level at
    // which we allocate storage for producer, and the loop level at
    // which we actually compute it. This unlocks a few optimizations.
    // producer.compute_at(consumer,x)方案与全部的内联方法非常的像。
    // 相反，让我们来识别在循环级中哪一层中我们为producer开辟空间，
    // 哪一层我们实际的进行计算，这会解锁一些优化方法。
    {
        Func producer("producer_root_y"), consumer("consumer_root_y");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y+1) +
                          producer(x+1, y) +
                          producer(x+1, y+1))/4;


        // Tell Halide to make a buffer to store all of producer at
        // the outermost level:
        // Halide在缓冲中存储所有的producer。
        producer.store_root();
        // ... but compute it as needed per y coordinate of the
        // consumer.
        // 但是计算的时候，只需要每一个y循环上的数值。
        producer.compute_at(consumer, y);

        producer.trace_stores();
        consumer.trace_stores();

        printf("\nEvaluating producer.store_root().compute_at(consumer, y)\n");
        consumer.realize(4, 4);

        // See below for a
        // visualization.
        // Reading the log or looking at the figure you should see
        // that producer and consumer again alternate on a
        // per-scanline basis. It computes a 5x2 box of the producer
        // to satisfy the first scanline of the consumer, but after
        // that it only computes a 5x1 box of the output for each new
        // scanline of the consumer!
        //可以看到producer和consumer交替计算。
        //首先计算5*2的producer区域来满足consumer的需求
        //接下来只要计算5*1的区间就够了。利用上次保存下来的一组数据就够了

        // Halide has detected that for all scanlines except for the
        // first, it can reuse the values already sitting in the
        // buffer we've allocated for producer. Let's look at the
        // equivalent C:
        // Halide除了第一行，每一行都会进行检测，可以重复利用上一次计算好的结果
        //

        float result[4][4];

        // producer.store_root() implies that storage goes here:
        float producer_storage[5][5];

        // There's an outer loop over scanlines of consumer:
        for (int y = 0; y < 4; y++) {

            // Compute enough of the producer to satisfy this scanline
            // of the consumer.
            for (int py = y; py < y + 2; py++) {

                // Skip over rows of producer that we've already
                // computed in a previous iteration.
                // 跳出producer的计算，因为上一次计算过了
                if (y > 0 && py == y) continue;


                for (int px = 0; px < 5; px++) {
                    producer_storage[py][px] = sin(px * py);
                }
            }

            // Compute a scanline of the consumer.
            for (int x = 0; x < 4; x++) {
                result[y][x] = (producer_storage[y][x] +
                                producer_storage[y+1][x] +
                                producer_storage[y][x+1] +
                                producer_storage[y+1][x+1])/4;
            }
        }

        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");
        

        // The performance characteristics of this strategy are pretty
        // good! The numbers are similar compute_root, except locality
        // is better. We're doing the minimum number of sin calls,
        // and we load values soon after they are stored, so we're
        // probably making good use of the cache:
        //这种策略非常的好！数据量和root方法是一样的，但是局部性更好
        //sin的调用次数很少，同时我们快速的加载计算出来的数据，这样我们可以
        //很好的利用cache。

        // producer.store_root().compute_at(consumer, y):
        // - Temporary memory allocated: 10 floats
        // - Loads: 64
        // - Stores: 39
        // - Calls to sin: 25
        // producer store_root && compute_at(comsumer,y)方案：
        // 临时空间的开辟 10个单元
        // 加载次数 4*4*4 = 64
        // 存储的次数：39次  16+23 
        // 调用sin次数：25次 10+5+5+5 

        // Note that my claimed amount of memory allocated doesn't
        // match the reference C code. Halide is performing one more
        // optimization under the hood. It folds the storage for the
        // producer down into a circular buffer of two
        // scanlines. Equivalent C would actually look like this:
        //实际上，内存的开辟与接下来的C语言不相同，Halide有更好的方案
        //只开辟两行的存储空间，不断的丢弃前一行不用的数据用来存放这一行的数据

        {
            // Actually store 2 scanlines instead of 5
            float producer_storage[2][5];
            for (int y = 0; y < 4; y++) {
                for (int py = y; py < y + 2; py++) {
                    if (y > 0 && py == y) continue;
                    for (int px = 0; px < 5; px++) {
                        // Stores to producer_storage have their y coordinate bit-masked.
                        producer_storage[py & 1][px] = sin(px * py);
                        //这个&用的很棒 0 & 1 = 0，  1 & 1 = 1 ，2 & 1 = 0，3 & 1 = 1
                        //结果只在0 1 之间循环，刚好只使用两行内存区间
                    }
                }

                // Compute a scanline of the consumer.
                for (int x = 0; x < 4; x++) {
                    // Loads from producer_storage have their y coordinate bit-masked.
                    result[y][x] = (producer_storage[y & 1][x] +
                                    producer_storage[(y+1) & 1][x] +
                                    producer_storage[y & 1][x+1] +
                                    producer_storage[(y+1) & 1][x+1])/4;
                }
            }
        }
    }

    // We can do even better, by leaving the storage outermost, but
    // moving the computation into the innermost loop:
    //我们还能够做的更好！将存储放在循环的外部，计算放在循环的内部
    {
        Func producer("producer_root_x"), consumer("consumer_root_x");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y+1) +
                          producer(x+1, y) +
                          producer(x+1, y+1))/4;


        // Store outermost, compute innermost.
        producer.store_root().compute_at(consumer, x); //这里循环的是x

        producer.trace_stores();
        consumer.trace_stores();

        printf("\nEvaluating producer.store_root().compute_at(consumer, x)\n");
        consumer.realize(4, 4);

        // See below for a
        // visualization.
        // You should see that producer and consumer now alternate on
        // a per-pixel basis. Here's the equivalent C:

        float result[4][4];

        // producer.store_root() implies that storage goes here, but
        // we can fold it down into a circular buffer of two
        // scanlines:
        float producer_storage[2][5];

        // For every pixel of the consumer:
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {

                // Compute enough of the producer to satisfy this
                // pixel of the consumer, but skip values that we've
                // already computed:
                // 第一次计算两行两列4个值，接下来每次只计算2个值
                if (y == 0 && x == 0)
                    producer_storage[y & 1][x] = sin(x*y);
                if (y == 0)
                    producer_storage[y & 1][x+1] = sin((x+1)*y);
                if (x == 0)
                    producer_storage[(y+1) & 1][x] = sin(x*(y+1));
                producer_storage[(y+1) & 1][x+1] = sin((x+1)*(y+1));

                result[y][x] = (producer_storage[y & 1][x] +
                                producer_storage[(y+1) & 1][x] +
                                producer_storage[y & 1][x+1] +
                                producer_storage[(y+1) & 1][x+1])/4;
            }
        }

        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");

        // The performance characteristics of this strategy are the
        // best so far. One of the four values of the producer we need
        // is probably still sitting in a register, so I won't count
        // it as a load:
        // producer.store_root().compute_at(consumer, x):
        // - Temporary memory allocated: 10 floats
        // - Loads: 48
        // - Stores: 56
        // - Calls to sin: 40
        // 采用这种方案更好，我们需要的4个值就在寄存器中，不需要再用load来加载
        // 临时存储空间： 10个单元
        // 加载：48次 
        // 存储：56次
        // sin调用：40次
    }

    // So what's the catch? Why not always do
    // producer.store_root().compute_at(consumer, x) for this type of
    // code?
    // 理论上这种方案最好，为什么不总是使用这种方法呢？
    // The answer is parallelism. In both of the previous two
    // strategies we've assumed that values computed on previous
    // iterations are lying around for us to reuse. This assumes that
    // previous values of x or y happened earlier in time and have
    // finished. This is not true if you parallelize or vectorize
    // either loop. Darn. If you parallelize, Halide won't inject the
    // optimizations that skip work already done if there's a parallel
    // loop in between the store_at level and the compute_at level,
    // and won't fold the storage down into a circular buffer either,
    // which makes our store_root pointless.
    // 答案是并行化多核因素。在前面的两种方案中，我们假设之前迭代计算的
    // 数据在当前数据的周围，可以直接拿过来重复使用。
    // 如果对于向量化和多核并行化循环就是不对的了，如果多核并行化，
    // Halide中，如果这种并行实在存储层级和计算层级，Halide不会注入已做好的优化，
    // 这使得store_root没有意义了。

    // We're running out of options. We can make new ones by
    // splitting. We can store_at or compute_at at the natural
    // variables of the consumer (x and y), or we can split x or y
    // into new inner and outer sub-variables and then schedule with
    // respect to those. We'll use this to express fusion in tiles:
    //我们通过分割方案来实现一种新的策略
    //我们操作store_at 和compute_at时利用consumer的自然变量x,y
    //或者，我们将 x,y分割成新的外不循环和内部循环然后再使用上述的方案。
    {
        Func producer("producer_tile"), consumer("consumer_tile");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y+1) +
                          producer(x+1, y) +
                          producer(x+1, y+1))/4;

        // We'll compute 8x8 of the consumer, in 4x4 tiles.
        Var x_outer, y_outer, x_inner, y_inner;
        consumer.tile(x, y, x_outer, y_outer, x_inner, y_inner, 4, 4);

        // Compute the producer per tile of the consumer
        producer.compute_at(consumer, x_outer);

        // Notice that I wrote my schedule starting from the end of
        // the pipeline (the consumer). This is because the schedule
        // for the producer refers to x_outer, which we introduced
        // when we tiled the consumer. You can write it in the other
        // order, but it tends to be harder to read.

        // Turn on tracing.
        producer.trace_stores();
        consumer.trace_stores();

        printf("\nEvaluating:\n"
               "consumer.tile(x, y, x_outer, y_outer, x_inner, y_inner, 4, 4);\n"
               "producer.compute_at(consumer, x_outer);\n");
        consumer.realize(8, 8);
        // The producer and consumer now alternate on a per-tile
        // basis. Here's the equivalent C:

        float result[8][8];

        // For every tile of the consumer:
        for (int y_outer = 0; y_outer < 2; y_outer++) {
            for (int x_outer = 0; x_outer < 2; x_outer++) {
                // Compute the x and y coords of the start of this tile.
                int x_base = x_outer*4;
                int y_base = y_outer*4;

                // Compute enough of producer to satisfy this tile. A
                // 4x4 tile of the consumer requires a 5x5 tile of the
                // producer.
                float producer_storage[5][5];
                for (int py = y_base; py < y_base + 5; py++) {
                    for (int px = x_base; px < x_base + 5; px++) {
                        producer_storage[py-y_base][px-x_base] = sin(px * py);
                    }
                }

                // Compute this tile of the consumer
                for (int y_inner = 0; y_inner < 4; y_inner++) {
                    for (int x_inner = 0; x_inner < 4; x_inner++) {
                        int x = x_base + x_inner;
                        int y = y_base + y_inner;
                        result[y][x] =
                            (producer_storage[y - y_base][x - x_base] +
                             producer_storage[y - y_base + 1][x - x_base] +
                             producer_storage[y - y_base][x - x_base + 1] +
                             producer_storage[y - y_base + 1][x - x_base + 1])/4;
                    }
                }
            }
        }

        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");

        // Tiling can make sense for problems like this one with
        // stencils that reach outwards in x and y. Each tile can be
        // computed independently in parallel, and the redundant work
        // done by each tile isn't so bad once the tiles get large
        // enough.
    }

    // Let's try a mixed strategy that combines what we have done with
    // splitting, parallelizing, and vectorizing. This is one that
    // often works well in practice for large images. If you
    // understand this schedule, then you understand 95% of scheduling
    // in Halide.
    // 最后是混合所有模式，对于大的图片使用，如果理解了这种方案，
    // 可以理解Halide中95%的策略。
    {
        Func producer("producer_mixed"), consumer("consumer_mixed");
        producer(x, y) = sin(x * y);
        consumer(x, y) = (producer(x, y) +
                          producer(x, y+1) +
                          producer(x+1, y) +
                          producer(x+1, y+1))/4;

        // Split the y coordinate of the consumer into strips of 16 scanlines:
        Var yo, yi;
        consumer.split(y, yo, yi, 16);
        //分割成16个小区域
        // Compute the strips using a thread pool and a task queue.
        consumer.parallel(yo);
        //线程并行
        // Vectorize across x by a factor of four.
        consumer.vectorize(x, 4);
        //向量化
        // Now store the producer per-strip. This will be 17 scanlines
        // of the producer (16+1), but hopefully it will fold down
        // into a circular buffer of two scanlines:
        producer.store_at(consumer, yo);
        // Within each strip, compute the producer per scanline of the
        // consumer, skipping work done on previous scanlines.
        producer.compute_at(consumer, yi);
        // Also vectorize the producer (because sin is vectorizable on x86 using SSE).
        producer.vectorize(x, 4);

        // Let's leave tracing off this time, because we're going to
        // evaluate over a larger image.
        // consumer.trace_stores();
        // producer.trace_stores();

        Buffer<float> halide_result = consumer.realize(160, 160);


        // Here's the equivalent (serial) C:

        float c_result[160][160];

        // For every strip of 16 scanlines (this loop is parallel in
        // the Halide version)
        for (int yo = 0; yo < 160/16 + 1; yo++) {

            // 16 doesn't divide 160, so push the last slice upwards
            // to fit within [0, 159] (see lesson 05).
            int y_base = yo * 16;
            if (y_base > 160-16) y_base = 160-16;

            // Allocate a two-scanline circular buffer for the producer
            float producer_storage[2][161];

            // For every scanline in the strip of 16:
            for (int yi = 0; yi < 16; yi++) {
                int y = y_base + yi;

                for (int py = y; py < y+2; py++) {
                    // Skip scanlines already computed *within this task*
                    if (yi > 0 && py == y) continue;

                    // Compute this scanline of the producer in 4-wide vectors
                    for (int x_vec = 0; x_vec < 160/4 + 1; x_vec++) {
                        int x_base = x_vec*4;
                        // 4 doesn't divide 161, so push the last vector left
                        // (see lesson 05).
                        if (x_base > 161 - 4) x_base = 161 - 4;
                        // If you're on x86, Halide generates SSE code for this part:
                        int x[] = {x_base, x_base + 1, x_base + 2, x_base + 3};
                        float vec[4] = {sinf(x[0] * py), sinf(x[1] * py),
                                        sinf(x[2] * py), sinf(x[3] * py)};
                        producer_storage[py & 1][x[0]] = vec[0];
                        producer_storage[py & 1][x[1]] = vec[1];
                        producer_storage[py & 1][x[2]] = vec[2];
                        producer_storage[py & 1][x[3]] = vec[3];
                    }
                }

                // Now compute consumer for this scanline:
                for (int x_vec = 0; x_vec < 160/4; x_vec++) {
                    int x_base = x_vec * 4;
                    // Again, Halide's equivalent here uses SSE.
                    int x[] = {x_base, x_base + 1, x_base + 2, x_base + 3};
                    float vec[] = {
                        (producer_storage[y & 1][x[0]] +
                         producer_storage[(y+1) & 1][x[0]] +
                         producer_storage[y & 1][x[0]+1] +
                         producer_storage[(y+1) & 1][x[0]+1])/4,
                        (producer_storage[y & 1][x[1]] +
                         producer_storage[(y+1) & 1][x[1]] +
                         producer_storage[y & 1][x[1]+1] +
                         producer_storage[(y+1) & 1][x[1]+1])/4,
                        (producer_storage[y & 1][x[2]] +
                         producer_storage[(y+1) & 1][x[2]] +
                         producer_storage[y & 1][x[2]+1] +
                         producer_storage[(y+1) & 1][x[2]+1])/4,
                        (producer_storage[y & 1][x[3]] +
                         producer_storage[(y+1) & 1][x[3]] +
                         producer_storage[y & 1][x[3]+1] +
                         producer_storage[(y+1) & 1][x[3]+1])/4};

                    c_result[y][x[0]] = vec[0];
                    c_result[y][x[1]] = vec[1];
                    c_result[y][x[2]] = vec[2];
                    c_result[y][x[3]] = vec[3];
                }

            }
        }
        printf("Pseudo-code for the schedule:\n");
        consumer.print_loop_nest();
        printf("\n");
        // Look on my code, ye mighty, and despair!

        // Let's check the C result against the Halide result. Doing
        // this I found several bugs in my C implementation, which
        // should tell you something.
        for (int y = 0; y < 160; y++) {
            for (int x = 0; x < 160; x++) {
                float error = halide_result(x, y) - c_result[y][x];
                // It's floating-point math, so we'll allow some slop:
                if (error < -0.001f || error > 0.001f) {
                    printf("halide_result(%d, %d) = %f instead of %f\n",
                           x, y, halide_result(x, y), c_result[y][x]);
                    return -1;
                }
            }
        }

    }

    // This stuff is hard. We ended up in a three-way trade-off
    // between memory bandwidth, redundant work, and
    // parallelism. Halide can't make the correct choice for you
    // automatically (sorry). Instead it tries to make it easier for
    // you to explore various options, without messing up your
    // program. In fact, Halide promises that scheduling calls like
    // compute_root won't change the meaning of your algorithm -- you
    // should get the same bits back no matter how you schedule
    // things.
    // 这种策略非常的困难，我们以三种在内存带宽冗余工作和并行化的平衡来实现
    // Halide不会自动的正确生成你想要的策略，但是尝试找到最好策略是
    // 比较容易的，在此过程中不会对程序产生影响
    //
    // So be empirical! Experiment with various schedules and keep a
    // log of performance. Form hypotheses and then try to prove
    // yourself wrong. Don't assume that you just need to vectorize
    // your code by a factor of four and run it on eight cores and
    // you'll get 32x faster. This almost never works. Modern systems
    // are complex enough that you can't predict performance reliably
    // without running your code.
    // 所以需要不断的实验尝试，不要想着简单的向量化和多核化就能实现32倍以上的
    // 加速效果。

    // We suggest you start by scheduling all of your non-trivial
    // stages compute_root, and then work from the end of the pipeline
    // upwards, inlining, parallelizing, and vectorizing each stage in
    // turn until you reach the top.

    // Halide is not just about vectorizing and parallelizing your
    // code. That's not enough to get you very far. Halide is about
    // giving you tools that help you quickly explore different
    // trade-offs between locality, redundant work, and parallelism,
    // without messing up the actual result you're trying to compute.
    //Halide不是简单的向量化和并行化你的代码，这还不够快，Halide是提供
    //一种工具帮助你快速的找到在局部性，冗余性，并行化三个方向最好的方案
    printf("Success!\n");
    return 0;
}
