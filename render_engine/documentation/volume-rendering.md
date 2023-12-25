# Volume Rendering

## Status

accepted

## Context

Volume Rendering is a big topic. There are many open questions when a volume renderer is implemented.

One main question is how to do the ray marching.
There are several discussion on the internet weather the
 - one pass: having one compute kernel that computes the cube-view_ray intersection and then one fragment shader to draw it.
 - two pass: render a front face cube and a back face cube and using its result as a ray direction in the fragment shader.

More information which is better:
 - https://www.nvidia.com/content/gtc/documents/1102_gtc09.pdf
 - https://www.scitepress.org/papers/2015/53447/53447.pdf

 When the basic rendering is done with Signed Distance Field one can add some performance improvement and also some shading effect like AO. But it is a different topic.

## Decision

- Go with a two pass solution first for experimental reasons. 
- Implement it as subpasses and subpass fetch. The same texture coordinate needs to be read back from the previous render passes. Thus, not need to do a full render pass to render the result. 

## Consequences

- Due to subpass-fetch, if tile rendering is used by the GPU it can have performance benefits.
- Two pass renderer for testing the current architecture
- Further improvements with introducing cuda task or vulkan compute task to calculate SDF.