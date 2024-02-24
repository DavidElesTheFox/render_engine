# Resource Uploader

## Status

accepted

## Context

Uploading textures/buffers needs to be done in a well synchronized fashion to not interfier with ongoing rendering commands. To achieve this DataTransferTasks need to be introduced.

The idea is to handle data upload/download in the same way how the render graph works. A DataTransferTask also waits and signals semaphors to achieve max perfromance on the GPU and it can be also part of the render graph. It Uses its own CommandContext to fully use the CPU. During the game logic one can add/remove objects from a staging area. This staging area later will cleared and synchronized to the GPU with DataTransferTasks. 

When data needs to be downloaded it uses std::futures to block only the main thread when it is needed. 

## Decision

DataTransferScheduler is the object that responsible to schedule the different upload/download tasks execution. With SyncOperations it synchronizes the upload and download tasks with other queue operations.

A device has a DataTransferScheduler (so it is not per Window). The execution of these operations needs to be done one certain point. It needs to be also part of the RenderGraph because these tasks are highly depend on the render execution.

## Consequences

- Textures and Buffers cannot be uploaded at any time. 
- Textures and Buffers has proper synchronization
- Textures and Buffers are the owners of the task that operates on the GPU. In this way it can be always queried what is the situation with the resource.
