# render_engine
Experimental vulkan render engine
The goal is to create a flexible render engine that 
 - supports multiple windows
 - place for making experimental render code.

## Dependencies
Library uses 
 - volk for vulkan loading
 - imgui as debug UI.
 - glm as math library
 - ADR templates for documentation of design decisions
 - sdl for window handling
 - CT image source: https://isbweb.org/data/vsj/ (Thank you)
   - exported to image with: https://www.radiantviewer.com/

# Multiple Window Support

A render engine is not only for games. Many scientific code uses rendering and compute. Sometimes the only goal is debugging some algorithm. For this purpose multiple window support is essential. 

This render API supports this need. Imgui is integrated in a way that it supports multiple context. With different renderer definition one can visualize different things on different windows.
![multiple window support](./render_engine/documentation/images/multi_window.png)

Open TODOs:
- Proper support of multi GPUs. The API uses volk. But in a wrong way and globally loads the features. It needs to be changed

# Offscreen rendering

Sometimes it is not just about showing the result but saving it or forwarding it to other application. For this purpose this API supports headless rendering. Later on this can be used also to visualize it. 

In this way one can distribute the load between a powerfull GPU and a less effective one (like an integrated). I.e.: Rendering and compute on the powerfull machine and then saving the image and visualizing it with an integrated GPU

Open TODOs:
- Proper multi queue support
- Parallel rendering support

# Volume Rendering

Volume renderer is added to the API. Mostly for experimental reasons. Like computing AO.
It is also a nice test of how to integrate Cuda compute inside rendering process. And using subpassload to achive better performance for ray marching. See more details in the Design decision section.

A simple volume rendering without AO
![](./render_engine/documentation/images/volume_rendering_ao.png)

And with AO:
![](./render_engine/documentation/images/volume_rendering_no_ao.png)

 # Design decisions

 There are some major design decision what was made during the library creation. Some of those are experimental decisions to figure out
 its consecvencies. 

 See 
  - [Error handling](render_engine/documentation/handling-errors.md)
  - [Object initialization](render_engine/documentation/object-initializations.md)
  - [Objects responsibility](render_engine/documentation/objects-responsibility.md)
  - [Singletons](render_engine/documentation/singletons.md)
  - [Resource State Machine](render_engine/documentation/resource-state-machine.md)
  - [Off-Screen Rendering](render_engine/documentation/offscreen-rendering.md)
  - [DescriptorSetLayout Setup](render_engine/documentation/descriptor-set-layout-setup.md)
  - [Volume Rendering](render_engine/documentation/volume-rendering.md)
  - [Distance Field Calculation and Compute](render_engine/documentation/distance-field-calculation.md)
  - [Volume Rendering with Ambient Occlusion](render_engine/documentation/volume-rendering_ao.md)
  - [Synchronization with fences,semaphores](render_engine/documentation/synchronization-primitives.md)