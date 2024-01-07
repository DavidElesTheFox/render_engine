# Distance Field Calculation

## Status

accepted

## Context

For Volume Rendering a distance field of a CT image can be really useful. 
 - It can speed up the ray marching - not fixed step size but step with the closest point distance
 - AO can be easily implemented - see more https://www.alanzucconi.com/2016/07/01/ambient-occlusion/

For this compute needs to be used. There are some question regarding this
 - Vulkan or Cuda?
 - Signed Distance Field or Sign is not important?
 - How to do synchronization?

## Decision

### Vulkan or Cuda?

Cuda is really widely used for compute tasks. So, it would be a nice experimental to use cuda-vulkan interoperability. 

So let's use cuda first and later maybe also vulkan.

But the story is not over yet. How to use cuda? Vulkan 1.3 can call cuda kernels within a submit queue. (see more https://www.phoronix.com/news/Vulkan-1.3.269-Released#google_vignette). It requires:
 - C syntax
 - input and output needs to be a stream

It means that calling cuda functions like radix sort etc is not possible, because there is no direct access to the kernels. Newer releases of Thrust library makes available the kernels, but no guarantee that these are compatible with the vulkan call requirements (https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_NV_cuda_kernel_launch.html).

Thus, let's not call the kernels from a vulkan queue but keep the input parameters as StreamObjects. It has many benefits using the streaming API. Like a stream can be a wrapper around a cuda 3d array or a vulkan texture as well. It makes easier the testing.

### Signed Distance Field?

For AO the sign is not neccessary in the distance field. Because we only need to know that close to the object the other points how close are. For this, Sign is not necessary. Also for the the ray marching speed up - probably it is safer with a sign distance field but it is not neccessary.

So, no sign required. That means that the problem what we need to solve is a
nearest neighbor problem.

#### Approximated Nearest Neighbor (ANN)

After a lot of googling all the implementation of a k nearest neighbor problem is based on Chunk's unpublished solution:
https://tmc.web.engr.illinois.edu/pub_ann.html?fbclid=IwAR26zQSewG3ohCCOJMRL7pgGDba58fo28jCuoyvirs9ZnqQUP4bjoV5dyVg (A minimalist's implementation of an approximate nearest neighbor algorithm in fixed dimensions)

Here also a short implementation is published as well. This works well on CPU but the question is how to implement on GPU. A nice implementation idea is given by NVidia: https://developer.nvidia.com/blog/thinking-parallel-part-iii-tree-construction-gpu/

Here a BVH is generated but the same can be done for ANN. The algorithm will look like:

- Use Radix Sort of 3d [0,1] normalized coordinates with the help of the 30 bit Morton code. It can be done on the GPU parallel. (CUB is integrated into the Cuda library so its radix sort can be used)
- Use Chun method to solve the ANN for all the points in the 'texture'. 

##### About Morton code
Morton code is used to make a 1D representation of 3D points but keeping the speasial property, i.e.: If a point is close in 3D then it should be close in 1D representation too. Morton code knows this, basically it is a natural octree. (https://en.wikipedia.org/wiki/Z-order_curve). Fun fact other similar representation is the Hilbert curve.

30bit Morton code. Chun's method and Morton code operates on integer numbers. But we have [0,1] normalized numbers. We are doing the same as it is recommended by the Nvidia blog and projecting our numbers from [0,1] to [0, 1023]. 
A bit more about this representation: https://www.forceflow.be/2013/10/07/morton-encodingdecoding-through-bit-interleaving-implementations/

Here an interesting question can be found in the comments. Why do we need to create a 30 bit representation of the numbers when Chun has a method to compare integer numbers based on these's morton code with a xor operator (less_msb function on Chun's code).

The answer is behind the radix sort. Radix sort is based on the number radix representation. If we want to do the sort on the GPU we need to create the proper representation and a compare function is not enough. 
### CudaCompute API

The goal is to create a Parallel API where compute tasks can be called parallel. 

With the cuda streaming API one can add custom commands after a kernel execution. This can be used later to signal Timeline Semaphores.

Vulkan images can be used with the streaming API (https://stackoverflow.com/questions/55424875/use-vulkan-vkimage-as-a-cuda-cuarray)

Cuda device and Vulkan device can be matched by UUID. See https://web.engr.oregonstate.edu/~mjb/cs575/CUDA_C_Programming_Guide.pdf (3.3.1. Vulcan Interoperability)

The idea is that each CudaDevice has several streams. Its owner can decide how many. 

Each ComputeTask is a not blocking task. Has a result which is a future.

A ComputeTask is executed in a CudaDevice using an available stream. 

In this way anyone can start a compute task, that:
 - not blocking the render task.
 - can do synchronization with time line semaphors
 - can be controlled which compute task is running on which vulkan device.
 - ownership question is decided on the caller side.
## Consequences

- Cuda support in the engine
- Non blocking cuda tasks.
- Cuda tasks can be synchronized with vulkan queues with timeline semaphores
- Cuda can use the vulkan textures
- Distance Field Calculation is running fully on the GPU.
- AO effect can be added to Volume Rendering
- Volume Rendering ray marching can be improved