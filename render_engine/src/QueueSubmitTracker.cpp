#include <render_engine/QueueSubmitTracker.h>

namespace RenderEngine
{
    QueueSubmitTracker::~QueueSubmitTracker()
    {
        if (_logical_device == nullptr)
        {
            return;
        }
        clear();
    }

    void QueueSubmitTracker::queueSubmit(VkSubmitInfo2&& submit_info,
                                         const SyncOperations& sync_operations,
                                         AbstractCommandContext& command_context)
    {
        VkFenceCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        VkFence fence{ VK_NULL_HANDLE };
        if ((*_logical_device)->vkCreateFence(**_logical_device, &create_info, nullptr, &fence) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot create fence for queue submit tracking");
        }

        command_context.queueSubmit(std::move(submit_info), sync_operations, fence);
        _fences.emplace_back(fence);
    }

    void QueueSubmitTracker::wait()
    {
        if (_fences.empty())
        {
            return;
        }
        if ((*_logical_device)->vkWaitForFences(**_logical_device, static_cast<uint32_t>(_fences.size()), _fences.data(), VK_TRUE, UINT64_MAX))
        {
            throw std::runtime_error("Cannot wait for fences");
        }
    }

    uint32_t QueueSubmitTracker::queryNumOfSuccess() const
    {
        uint32_t result{ 0 };
        for (auto fence : _fences)
        {
            if ((*_logical_device)->vkGetFenceStatus(**_logical_device, fence) == VK_SUCCESS)
            {
                result++;
            }
        }
        return result;
    }
    void QueueSubmitTracker::clear()
    {
        wait();
        for (auto fence : _fences)
        {
            (*_logical_device)->vkDestroyFence(**_logical_device, fence, nullptr);
        }
        _fences.clear();
    }
}