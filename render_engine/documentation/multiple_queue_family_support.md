# Multiple Queue Family Support

## Status

accepted

## Context

In order to fully utilyze the GPU not just multiple queue support is required but also multi queue families. The motivation is fairly simple, 
when we would like to do data transfer why we would use a queue that supports the complex rendering commands if we need 'only' data copy. For this
use case less complex queues are also feasable.

The tricky part is the ownership transition. Although, a buffer and a texture can also be marked as a shared resource between queue families it harms to 
the performance. Exclusive access is usually the prefarable way to use these resources. (See more details in vulkan API VK_SHARING_MODE section).
## Decision

The resource state machine currently the responsible for memory barrier handlings ([ADR document](./resource-state-machine.md)). Thus, it is a logical
place to dealing with execution barriers and also queue family ownership transforms.

### Data Transfer
The most tipical scenario when multiple queue family is required when data is uploaded/downloaded. When a specific transfer queue is used the simple things becomes more complex.

![Overview about the process](./umlet/data_transfer.png)

#### Dealing with binary semaphores

When the transfer needs to wait for other operations that uses binary semaphores on the source queue we need to wait them (i.e. Binary semaphores needs to be waited on the same queue.). To achieve this before starting the data transfer these operations needs to be waited on the source queue during Queu Family Ownership Transfer or with a execution barrier.

#### Dealing with initial transfer

When a resource is used for the first time it hasn't got any assigned queue family. During this case the Transfer queue can be handle as an owner.

#### Queue Family Ownership Transfer

It has two parts:
- Release operation on the Source Queue. 
- Acquire operation on the Destination Queue.

To achive good performance during queue family ownership transfer it is possible to do image layout transition or define a pipeline stage where we would like to make the memory available. But, it is not obvious that it can be done. Like not all the queue families supports all the pipeline stages. For example if the resource is already in a transfer queue. And the target stage is a Fragment Shader Stage the barrier cannot be applied. In this situation two barrier needs to be execute on the destination queue. One Acquire and then a pipeline barrier with a proper stage.

![Overview about the process](./umlet/queue_ownership_transfer.png)

## Consequences

More queue family support means more queue support. More queue support means 
- more parallel execution on the CPU. So, the CPU is utilized more
- The GPU cores also can be utilized better when the proper queues are used for the proper tasks.
- More complex code.
- Many semaphore creation and SyncObject/SyncOperation. It can be problem for the allocations. Probably custom allocator and object pool needs to be introduce to avoid memory fragmentation issues.