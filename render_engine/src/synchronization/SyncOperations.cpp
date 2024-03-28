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
        _wait_semaphore_container.emplace_back(submit_info.semaphore);
        _wait_semaphore_dependency.emplace_back(std::move(submit_info));
    }
    void SyncOperations::addWaitOperation(SyncPrimitives& sync_object, const std::string& semaphore_name, VkPipelineStageFlags2 stage_mask, uint64_t value)
    {
        VkSemaphoreSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submit_info.semaphore = sync_object.getSemaphore(semaphore_name);
        submit_info.stageMask = stage_mask;
        submit_info.value = sync_object.getTimelineOffset(semaphore_name) + value;
        _wait_semaphore_container.emplace_back(submit_info.semaphore);
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

    void SyncOperations::HostOperations::addSemaphore(std::string name, VkSemaphore semaphore)
    {
        assert(_registered_semaphores.contains(name) == false);
        _registered_semaphores.emplace(std::move(name), semaphore);
    }

    void SyncOperations::HostOperations::addSemaphore(SyncPrimitives& sync_object, const std::string& semaphore_name)
    {
        addSemaphore(semaphore_name, sync_object.getSemaphore(semaphore_name));
    }

    std::pair<VkResult, uint32_t> SyncOperations::HostOperations::acquireNextSwapChainImage(LogicalDevice& logical_device,
                                                                                            VkSwapchainKHR swap_chain,
                                                                                            const std::string& semaphore_name) const
    {
        uint32_t image_index = 0;
        auto call_result = logical_device->vkAcquireNextImageKHR(*logical_device,
                                                                 swap_chain,
                                                                 UINT64_MAX,
                                                                 _registered_semaphores.at(semaphore_name),
                                                                 VK_NULL_HANDLE,
                                                                 &image_index);
        return { call_result, image_index };
    }

    void SyncOperations::HostOperations::unionWith(const HostOperations& o)
    {
        for (auto [name, semaphore] : o._registered_semaphores)
        {
            addSemaphore(name, semaphore);
        }
    }

    const SyncOperations& SyncOperations::fillInfo(VkSubmitInfo2& submit_info) const
    {
        submit_info.waitSemaphoreInfoCount = static_cast<uint32_t>(_wait_semaphore_dependency.size());
        submit_info.pWaitSemaphoreInfos = _wait_semaphore_dependency.data();

        submit_info.signalSemaphoreInfoCount = static_cast<uint32_t>(_signal_semaphore_dependency.size());
        submit_info.pSignalSemaphoreInfos = _signal_semaphore_dependency.data();
        return *this;
    }

    const SyncOperations& SyncOperations::fillInfo(VkPresentInfoKHR& present_info) const
    {
        present_info.waitSemaphoreCount = static_cast<uint32_t>(_wait_semaphore_container.size());
        present_info.pWaitSemaphores = _wait_semaphore_container.data();
        return *this;
    }
    void SyncOperations::unionWith(const SyncOperations& o)
    {
        std::ranges::copy(o._wait_semaphore_dependency, std::back_inserter(_wait_semaphore_dependency));
        std::ranges::copy(o._wait_semaphore_container, std::back_inserter(_wait_semaphore_container));
        std::ranges::copy(o._signal_semaphore_dependency, std::back_inserter(_signal_semaphore_dependency));
        _host_operations.unionWith(o._host_operations);
    }
    void SyncOperations::shiftTimelineSemaphoreValues(uint64_t offset, VkSemaphore semaphore)
    {
        for (auto& submit_info : _signal_semaphore_dependency)
        {
            if (submit_info.semaphore != semaphore)
            {
                continue;
            }
            submit_info.value += offset;
        }
        for (auto& submit_info : _wait_semaphore_dependency)
        {
            if (submit_info.semaphore != semaphore)
            {
                continue;
            }
            submit_info.value += offset;
        }
    }
    void SyncOperations::clear()
    {
        _wait_semaphore_dependency.clear();
        _wait_semaphore_container.clear();
        _signal_semaphore_dependency.clear();
        _host_operations.clear();
    }
    SyncOperations SyncOperations::restrict(const AbstractCommandContext& context) const
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