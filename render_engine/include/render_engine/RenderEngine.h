#pragma once

#include <volk.h>

#include <render_engine/CommandContext.h>
#include <render_engine/Device.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/synchronization/SyncOperations.h>
#include <render_engine/TransferEngine.h>

#include <unordered_map>

#include <algorithm>
#include <ranges>

namespace RenderEngine
{
    class AbstractRenderer;

    class RenderEngine
    {
    public:

        RenderEngine(Device& device,
                     std::shared_ptr<SingleShotCommandContext>&& single_shot_command_context,
                     std::shared_ptr<CommandContext>&& command_context,
                     uint32_t back_buffer_count);

        void onFrameBegin(const std::ranges::input_range auto& renderers, uint32_t image_index)
        {
            for (auto* renderer : renderers)
            {
                renderer->onFrameBegin(image_index);
            }
        }
        [[nodiscard]]
        bool render(const SyncOperations& sync_operations,
                    const std::ranges::input_range auto& renderers,
                    uint32_t image_index)
        {
            std::vector<VkCommandBufferSubmitInfo> command_buffer_infos = executeDrawCalls(renderers, image_index);
            // TODO: Do not synchronize per window draw rather once per all widnow draw. Currently it is necessary to start the uploads properly.
            _device.getDataTransferContext().synchronizeScheduler({});

            auto all_sync_operations = collectSynchronizationOperations(renderers, sync_operations, image_index);

            if (command_buffer_infos.empty() == false)
            {
                submitDrawCalls(command_buffer_infos, all_sync_operations);
                return true;
            }
            return false;
        }

        GpuResourceManager& getGpuResourceManager() { return _gpu_resource_manager; }
        uint32_t getBackBufferSize() const { return _gpu_resource_manager.getBackBufferSize(); }
        TransferEngine& getTransferEngine() { return _transfer_engine; }
        CommandContext& getCommandContext() { return *_command_context; }
        SingleShotCommandContext& getTransferCommandContext();
    private:
        std::vector<VkCommandBufferSubmitInfo> executeDrawCalls(const std::ranges::input_range auto& renderers,
                                                                uint32_t image_index)
        {
            std::vector<VkCommandBufferSubmitInfo> command_buffers;
            for (AbstractRenderer* drawer : renderers)
            {
                drawer->draw(image_index);
                auto current_command_buffers = drawer->getCommandBuffers(image_index);
                std::ranges::transform(current_command_buffers,
                                       std::back_inserter(command_buffers),
                                       [](const auto& command_buffer)
                                       {
                                           return VkCommandBufferSubmitInfo{
                                                               .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                                                               .pNext = VK_NULL_HANDLE,
                                                               .commandBuffer = command_buffer,
                                                               .deviceMask = 0 };
                                       });
            }
            return command_buffers;
        }

        SyncOperations collectSynchronizationOperations(const std::ranges::input_range auto& renderers,
                                                        const SyncOperations& sync_operations,
                                                        uint32_t image_index)
        {
            SyncOperations result = sync_operations;
            for (AbstractRenderer* drawer : renderers)
            {
                auto renderer_sync_operations = drawer->getSyncOperations(image_index);
                result = result.createUnionWith(renderer_sync_operations);
            }
            return result;
        }

        void submitDrawCalls(const std::vector<VkCommandBufferSubmitInfo>& command_buffers,
                             const SyncOperations& sync_operations);


        Device& _device;
        GpuResourceManager _gpu_resource_manager;
        // TODO make borrow_ptr
        std::shared_ptr<CommandContext> _command_context;
        TransferEngine _transfer_engine;
    };
}