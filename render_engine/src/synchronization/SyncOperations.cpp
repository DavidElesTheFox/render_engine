#include <render_engine/synchronization/SyncOperations.h>

#include <render_engine/RenderContext.h>

#include <cassert>
#include <ranges>

namespace RenderEngine
{
    void SyncOperations::addWaitOperation(SyncPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
    {
        VkSemaphoreSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
        submit_info.stageMask = stage_mask;
        _wait_semaphore_dependency.emplace_back(std::move(submit_info));
    }
    void SyncOperations::addWaitOperation(SyncPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
    {
        VkSemaphoreSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
        submit_info.stageMask = stage_mask;
        submit_info.value = sync_object.getTimelineOffset(semaphore_name) + value;
        _wait_semaphore_dependency.emplace_back(std::move(submit_info));
    }
    void SyncOperations::addSignalOperation(SyncPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask)
    {
        VkSemaphoreSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
        submit_info.stageMask = stage_mask;
        _signal_semaphore_dependency.emplace_back(std::move(submit_info));
    }
    void SyncOperations::addSignalOperation(SyncPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
    {
        VkSemaphoreSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
        submit_info.stageMask = stage_mask;
        submit_info.value = sync_object.getTimelineOffset(semaphore_name) + value;
        _signal_semaphore_dependency.emplace_back(std::move(submit_info));
    }
    const SyncOperations& SyncOperations::fillInfo(VkSubmitInfo2& submit_info) const
    {
        submit_info.waitSemaphoreInfoCount = _wait_semaphore_dependency.size();
        submit_info.pWaitSemaphoreInfos = _wait_semaphore_dependency.data();

        submit_info.signalSemaphoreInfoCount = _signal_semaphore_dependency.size();
        submit_info.pSignalSemaphoreInfos = _signal_semaphore_dependency.data();
        return *this;
    }
    void SyncOperations::unionWith(const SyncOperations& o)
    {
        uint32_t wait_semaphore_offset = _wait_semaphore_dependency.size();
        uint32_t signal_semaphore_offset = _signal_semaphore_dependency.size();

        std::ranges::copy(o._wait_semaphore_dependency, std::back_inserter(_wait_semaphore_dependency));
        std::ranges::copy(o._signal_semaphore_dependency, std::back_inserter(_signal_semaphore_dependency));
        assert((_fence == VK_NULL_HANDLE || o._fence == VK_NULL_HANDLE) && "fence conflict");
        if (_fence == VK_NULL_HANDLE)
        {
            _fence = o._fence;
        }
    }
    void SyncOperations::shiftTimelineSemaphoreValues(uint64_t offset)
    {
        for (auto& submit_info : _signal_semaphore_dependency)
        {
            // Shifting all values is not an issue, because this field is used only for timeline semaphores
            submit_info.value += offset;
        }
        for (auto& submit_info : _wait_semaphore_dependency)
        {
            // Shifting all values is not an issue, because this field is used only for timeline semaphores
            submit_info.value += offset;
        }
    }
    void SyncOperations::clear()
    {
        _wait_semaphore_dependency.clear();
        _signal_semaphore_dependency.clear();
        _fence = VK_NULL_HANDLE;
    }
    SyncOperations SyncOperations::restrict(const CommandContext& context) const
    {
        auto result = *this;
        std::erase_if(result._wait_semaphore_dependency, [&](auto& submit_info)
                      {
                          return context.isPipelineStageSupported(submit_info.stageMask) == false;
                      });
        std::erase_if(result._signal_semaphore_dependency, [&](auto& submit_info)
                      {
                          return context.isPipelineStageSupported(submit_info.stageMask) == false;
                      });
        return result;
    }

}