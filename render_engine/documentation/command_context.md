# Command Context

## Status

accepted

## Context

When queue is used for issuing command there are some information that might be required to execute the command properly. To support this need the CommnandContext class is created.

Its goal to hold the information about the given queue and allow to create different command buffers for different purpuses. For example one can query what are the supported pipeline stages by the 
given command context. This is extremly useful when one need to use semaphores between different queues. 

## Decision

CommandContext is the object how one can create command buffers. With this it is guaranteed that all the information is available for the correct usage of the command buffer.

## Consequences

- Proper lifetime support of queues
- Queue Family and feature information is available when command buffers are used. Thus, one can guarantee correctness.
