# Resource Uploader

## Status

proposed

## Context

Uploading/downloading data is quite suboptimal currently. There are some issues with it:
 - Each Texture has its own staging area. Even if it is never uploaded or downloaded
 - There is a queue submission for each texture. Rather collecting all the textures and execute the upload with one queue submittion
 
Idea for imporvements:
 - Textures has no more Staging Area. Rather the staging area is created when the upload task is executed (simirarly how it works for Buffers.)
 - Textures has only one Staging Area. Its size is the overall size of the textures. It will result less allocation. Same for Buffers
 - Textures are collected and the upload is done in one submition.

## Decision

## Consequences
