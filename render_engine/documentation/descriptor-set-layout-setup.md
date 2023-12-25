# DescriptorSetLayout Setup

## Status

accepted

## Context

Vulkan DescriptorSet basically defines a set of GPU resource. A Layout defines how to access a banch of these resources. Later on 
these layouts will be bind to the pipeline. 

Thus, it can be important how frequently these descriptors are changing. When these are the same or not changing at all the GPU might do some optimization. 
## Decision

To support this feature a GpuResourceSet class got introduced. It is basically a layout and its resources. So, during render different
descriptors are used for different update frequencies. 

A simple scenario can be when a technique is used by multiple renderers. In this way the binding of the the technique's constant resources will not cause extra overhead, only those will be 'rebinded' that are different.

See more about the bindings: https://developer.nvidia.com/vulkan-shader-resource-binding
## Consequences

- More clean API and clean up of the MaterialInstance::createTechnique function
- Proper lifetime management for the GpuResourceSet
- support reusing a techinque.
