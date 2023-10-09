# Objects Responsibility

## Status

Accepted

## Context

In game engine/render engines sometimes it is not clear what an objects responsibility and things are mixed up. Like having a shader that owns the code and the GPU shader module handler as well. In this case it is harder to track changes, reload dynamically etc.

## Decision

Objects needs to have a proper responsibility, thus data holder objects - assets - needs to hold data and that's all. Other object can be responsible to managing the corresponding gpu resource etc. The point is not mixing the two.

## Consequences

Easier asset management, like reloading, duplicating assets and its lifetimes.