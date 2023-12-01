#include <render_engine/resources/ResourceStateMachine.h>

#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/Texture.h>

#include <cassert>
#include <stdexcept>
namespace RenderEngine
{

    void ResourceStateMachine::recordStateChange(Texture* image, TextureState next_state)
    {
        _images[image] = std::move(next_state);
    }

    void ResourceStateMachine::recordStateChange(Buffer* buffer, BufferState next_state)
    {
        _buffers[buffer] = std::move(next_state);
    }

    void ResourceStateMachine::commitChanges(VkCommandBuffer command_buffer)
    {
        auto image_barriers = createImageBarriers();
        auto buffer_barriers = createBufferBarriers();
        if (image_barriers.empty() && buffer_barriers.empty())
        {
            return;
        }
        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.imageMemoryBarrierCount = image_barriers.size();
        dependency.pImageMemoryBarriers = image_barriers.data();
        dependency.bufferMemoryBarrierCount = buffer_barriers.size();
        dependency.pBufferMemoryBarriers = buffer_barriers.data();
        vkCmdPipelineBarrier2(command_buffer, &dependency);
    }

    std::vector<VkImageMemoryBarrier2> ResourceStateMachine::createImageBarriers()
    {
        std::vector<VkImageMemoryBarrier2> image_barriers;
        for (auto& [texture, next_state] : _images)
        {
            const auto& current_state = texture->getResourceState();

            if (next_state == std::nullopt
                || next_state == current_state)
            {
                continue;
            }
            VkImageMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
            barrier.image = texture->getVkImage();
            barrier.srcStageMask = current_state.pipeline_stage;
            barrier.srcAccessMask = current_state.access_flag;
            barrier.dstStageMask = next_state->pipeline_stage;
            barrier.dstAccessMask = next_state->access_flag;

            if (current_state.queue_family_index || next_state->queue_family_index)
            {
                assert(current_state.queue_family_index && next_state->queue_family_index
                       && "If one of the state has a queue family index then both needs to have");
                if (current_state.queue_family_index != next_state->queue_family_index)
                {
                    barrier.srcQueueFamilyIndex = *current_state.queue_family_index;
                    barrier.dstQueueFamilyIndex = *next_state->queue_family_index;
                }
            }

            barrier.oldLayout = current_state.layout;
            barrier.newLayout = next_state->layout;
            barrier.subresourceRange = texture->createSubresourceRange();
            image_barriers.emplace_back(std::move(barrier));
            texture->overrideResourceState(*next_state);
        }
        _images.clear();
        return image_barriers;
    }

    std::vector<VkBufferMemoryBarrier2> ResourceStateMachine::createBufferBarriers()
    {
        std::vector<VkBufferMemoryBarrier2> buffer_barriers;
        for (auto& [buffer, next_state] : _buffers)
        {
            const auto& current_state = buffer->getResourceState();

            if (next_state == std::nullopt
                || next_state == current_state)
            {
                continue;
            }
            VkBufferMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
            barrier.buffer = buffer->getBuffer();

            barrier.srcStageMask = current_state.pipeline_stage;
            barrier.srcAccessMask = current_state.access_flag;

            barrier.dstStageMask = next_state->pipeline_stage;
            barrier.dstAccessMask = next_state->access_flag;

            if (current_state.queue_family_index || next_state->queue_family_index)
            {
                assert(current_state.queue_family_index && next_state->queue_family_index
                       && "If one of the state has a queue family index then both needs to have");
                if (current_state.queue_family_index != next_state->queue_family_index)
                {
                    barrier.srcQueueFamilyIndex = *current_state.queue_family_index;
                    barrier.dstQueueFamilyIndex = *next_state->queue_family_index;
                }
            }
            barrier.offset = 0;
            barrier.size = buffer->getDeviceSize();
            buffer_barriers.emplace_back(std::move(barrier));
            buffer->overrideResourceState(*next_state);
        }
        _buffers.clear();
        return buffer_barriers;
    }


}