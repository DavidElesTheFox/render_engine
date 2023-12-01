# Resource State Machine

## Status

Prototype 

## Context

Handling GPU resource states on the GPU is hard. As a state change it requires synchronization and even using synch2 interface of 
vulkan it is hard to track properly all the state changes and also hard to read the code. Usually one need to maintain many redunant code.

The problem I believe nicely described here: https://themaister.net/blog/2017/08/15/render-graphs-and-vulkan-a-deep-dive/
and a bit more detailed version: https://themaister.net/blog/2019/04/

This resource state machine concept is a lightweight version of a render graph which operates only on resource level. 

Here is an example for a proper image transition (one can notice that queue family transition is not handled)

```
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.image = _texture;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    barrier.srcAccessMask = VK_ACCESS_2_NONE;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.subresourceRange = createSubresourceRange();
    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(command_buffer, &dependency);
}
{
    VkBufferImageCopy copy_region{};
    /*...*/
    vkCmdCopyBufferToImage(command_buffer,
        _staging_buffer.getBuffer(),
        _texture,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy_region);
}

{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.image = _texture;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    if (_shader_usage & VK_SHADER_STAGE_VERTEX_BIT)
    {
        barrier.dstStageMask |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    }
    if (_shader_usage & VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        barrier.dstStageMask |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    }
    barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = transfer_engine.getQueueFamilyIndex();
    barrier.dstQueueFamilyIndex = dst_queue_family_index;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.subresourceRange = createSubresourceRange();
    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(command_buffer, &dependency);
}
```

The goal is to end up with something like that:

```
ResourceStateMachine state_machine;
state_machine.recordStateChange(this,
    getResourceState().clone()
    .setPipelineStage(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
    .setAccessFlag(VK_ACCESS_2_TRANSFER_WRITE_BIT)
    .setImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    .setQueueFamilyIndex(transfer_engine.getQueueFamilyIndex()));
state_machine.commitChanges(command_buffer);

{
    VkBufferImageCopy copy_region{};
    /*...*/
    vkCmdCopyBufferToImage(command_buffer,
        _staging_buffer.getBuffer(),
        _texture,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy_region);
}

auto getFinalStageMask = [&] 
{
    VkPipelineStageFlagBits2 result{ 0 };
    if(_shader_usage & VK_SHADER_STAGE_VERTEX_BIT)
    {
        result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    }
    if(_shader_usage & VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    }
    return result;
};
state_machine.recordStateChange(this,
    getResourceState().clone()
    .setPipelineStage(getFinalStageMask())
    .setAccessFlag(VK_ACCESS_2_SHADER_READ_BIT)
    .setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    .setQueueFamilyIndex(dst_queue_family_index));
state_machine.commitChanges(command_buffer);
```
## Decision

Currently it is not part of the render graph, i.e. Chain of renderers. It is because it is a different level of abstraction.
Renderers is all about the "How"; resources are about the "What". And despite the resource transition is done by queues and 
command buffers it is just the channel of the communication and still not about rendering. Like during compute nothing is rendered
but still need to handle resource transitions. Therefore, separating these two concept into a different logical partitions makes perfect sense and gives more flexibility in the future.

## Consequences

- Not trivial synchronization between multiple resource state machines. 
- A bit confusing the resource state, while it is not applied until the command buffer is executed on the GPU. 
  It is a confusing fact, that is reflected on the API
- Easy to read what state changes are done.
