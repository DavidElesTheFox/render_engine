# Synchronization Primitives

## Status

accepted

## Context

Synchronization is one of the most difficult problem in rendering. Thus, a good API that supports the usage of the vulkan synchronization is crucial.

There are different synchronization methodes in vulkan how one can do synchronization between the Host and the Device or between devices.

One important aspect of the problem that synchronization is done by memory barriers. But Barriers are not just synchronizing the availability and visibility of the memory but also does different transitions (image layout, family queue ownership).

The API of barriers based on reasource state tracking. See more details about it in the [Resource State Machine ADR](./resource-state-machine.md).

This section is about how to organize the different synchronization operations when a command buffer is submitted. These synchronization can be:
 - Binary semaphores: Works inside a queue and has two states. It is resetted by the device after the wait evetns are finished. Thus, a binary semaphore can be reused.
 - Timeline semaphores: Works between queues as well. 
 Host can also trigger a semaphore. Timeline semaphores can be external and waited by for example a cuda kernel as well. Reset is not available. Its value must be increased by time.
 - Events: Can be triggered on the host or by a queue. And can be waited in a queue. Also, can be reset.
 - Fences: Device triggers it when a queue reaches it and can be waited on the Host side.

## Decision

All the different synchronization methods are distributed 3 different part in the API:
 - SyncPrimitives: Synchronization Primitives are the owner of the different Vulkan objects. It is responsible to the lifetime of the given semaphore/fence/event.
 - SyncOperations: Copyable collection of the operations. This contains the submit infos. One need to be careful that the corresponding primitives needs to be still alive when the sync opeation is called. This class responsible to store the submit informations and fill the queue submit infos.
 - SyncObject: This class is an aggregation of the synchronization primitives and operations. It contains by default multiple groups. The idea is that one configure a synchronization object with its semaphores and wait and signal operations. And later these are only queried during the submittion by group. For example an internal group of operations stores the signal operation of the rendering process. While the external group contains the corresponding wait operation.

## Consequences

- Semaphores are created once and later on reused. 
- Proper lifetime management of the objects. 
- Gives control above the reinitialization of the timeline semaphores. Timeline semaphores can be automatically shifted when its range is known. In this way can be reused until it reachis the UINT64_MAX. When it reachis its limit automatic recreation can be triggered and managed properly.
- In one place the Synchronization object can be configured. In this way it gives a nice overview the dependencies of the given gpu operation.
- Operations are detached from the primitives. It gives a nice flexibility of extending the operations that belongs to multiple gpu operations. Each RenderPass can has its own Synchronization Object but the all of these operations can be merged into one.