# Off-Screen Rendering

## Status

Prototype 

## Context

Using multiple GPUs for rendering can be really useful. For this it is essential to have a good abstraction above the render target.

The idea is to create a Window type which is headless. So, no real window instance is there in the background. This is the OffScreenWindow. It behaves like a normal window and renderers doesn't know exactly what is its render targets.

If it works well then two window can be linked together via an Image Stream.
The Off Screen window pushes its images to the image stream and the window that makes the presentation can use an image stream as a data input. 

The point is that the real window doesn't know what is behind the image stream. It maybe an other window, or ffmpeg and a video, or just static images.

In this way all the computation load can be pushed into a powerfull GPU and UI rendering can be on a lightweight GPU (like the integrated GPU).

## Decision

Create an OffScreenWindow and a WindowTunnel. WindowTunnel is the object that connects two Windows. (It even can be a chain of windows later).

The RenderTarget class gonna be the abstraction above a SwapChain or a set of textures.
## Consequences

- Possible load distribution between gpus
- Increased delay due to download.
- For having a good performance need to make parallel read back and render stage.
