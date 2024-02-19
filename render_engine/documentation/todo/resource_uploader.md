# Resource Uploader

## Status

proposed

## Context

Uploading textures/buffers needs to be done in a well synchronized fashion to not interfier with ongoing rendering commands. To achieve this DataTransferTasks need to be introduced.

The idea is to handle data upload/download in the same way how the render graph works. A DataTransferTask also waits and signals semaphors to achieve max perfromance on the GPU and it can be also part of the render graph. It Uses its own CommandContext to fully use the CPU. During the game logic one can add/remove objects from a staging area. This staging area later will cleared and synchronized to the GPU with DataTransferTasks. 

When data needs to be downloaded it uses std::futures to block only the main thread when it is needed. 

## Decision

## Consequences
