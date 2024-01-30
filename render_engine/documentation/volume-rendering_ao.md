# Volume Rendering AO

## Status

accepted

## Context

Details how the volume rendering is implemented can be checked here: [ADR documentation](./volume-rendering.md)

For an improved ray marching technique an SDF (Signed Distance Field) can help a lot. I.e.: Instead of using a fixed step size for ray marching always make a step based on the closest point on the grid.

For Ambient Occlusion calculation SDF is not necessary, because a simple distance field (nearest neighbor) is enough. [The idea is well described here.](https://www.alanzucconi.com/2016/07/01/ambient-occlusion/)

For distance field calculation the [ADR documentation](./distance-field-calculation.md) is ready. 

The question now, whether a realtime AO can be calculated or not for a smaller CT image, like 256x256x100.

## Decision

Using a simple Distance Field calculation for AO based on the description. Then we can use the step size from this value. Due to interpolation errors and avoiding the tunneling issue (our distance field has no sign, does we don't know when we went through on our surface), the volume rendering algorithm is changed in the way that only a fractional part is stepped of the whole distance (i.e 80%).

For AO this distance field is used to calculate how much should be a point is shaded.

## Consequences

The current performance of the Distance Field calculation kernel is pretty good but still doesn't realtime. 

Applied optimization: 
 - Instead of recursive algorithm using a stacked base solution (with a maximum limit of the stack size)
 - Memory access is globally changed (later on maybe it worth to create separated compile units from the different kernels). The goal is to improve between L1 and L2 cachmisses by invalidating L1 cache completly. In this way no cachline is used that makes faster the uncoalesced memory access. [More detailes about it](https://stackoverflow.com/questions/15128310/un-coalesced-global-memory-access-caused-by-indirect-access-in-cuda)
 - Increase register preassure: Removing lambdas, and using macros instead. 

 Current bottleneck: Due to the 50 register usage the scheduler cannot schedule more warps and due to this the throughput cannot be as good as it should be.

 Currently the whole computation takes 200ms. That's the reason of the 4 FPS. A streaming like architecture to updating the Distance Field can be a proper solution (i.e.: Instead of waiting for the next frame reusing the previous frame).