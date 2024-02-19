# Parallel Rendering

## Status

proposal

## Context

Rendering with vulkan can be done by multiple threads in a parallel fashion. To utilize fully the CPU draw calls and upload commands can be executed parallel.

Each thread can has its own CommandContext ([See more about it in ADR](../command_context.md)) (own queue) and can record command there. The idea is to create a pool of the CommandContexts and each thread can request a context based on its thread id. 

The idea is that each renderer has a predifined number of threads where it issues draw calls. Each thread requests a CommandContext based on its thread id and records draw calls or waits for available CommandContext.

Same will happen with upload and download data tasks, with the addition that it uses different queue families to not stall rendering threads.

## Decision

## Consequences
