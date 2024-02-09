#pragma once

#include <volk.h>

#include <render_engine/DeviceLookup.h>

#include <memory>
#include <vector>
namespace RenderEngine
{
    // TODO Rename to CommandUnit
    class CommandContext : public std::enable_shared_from_this<CommandContext>
    {
        struct CreationToken
        {};
    public:
        enum class Usage
        {
            SingleSubmit = 0,
            MultipleSubmit
        };
        static std::shared_ptr<CommandContext> create(VkDevice logical_device,
                                                      uint32_t queue_family_index,
                                                      DeviceLookup::QueueFamilyInfo queue_family_info)
        {
            return std::make_shared<CommandContext>(logical_device, queue_family_index, std::move(queue_family_info), CreationToken{});
        }

        CommandContext(VkDevice logical_device,
                       uint32_t queue_family_index,
                       DeviceLookup::QueueFamilyInfo queue_family_info,
                       CreationToken);

        CommandContext(CommandContext&&);
        CommandContext(const CommandContext&) = delete;

        CommandContext& operator=(CommandContext&&);
        CommandContext& operator=(const CommandContext&) = delete;

        ~CommandContext();
        VkQueue getQueue() const;
        uint32_t getQueueFamilyIndex() const { return _queue_family_index; }
        VkCommandBuffer createCommandBuffer(Usage usage) const;
        std::vector<VkCommandBuffer> createCommandBuffers(uint32_t count, Usage usage) const;

        std::shared_ptr<CommandContext> clone() const;

        VkDevice getLogicalDevice() { return _logical_device; }

        bool isPipelineStageSupported(VkPipelineStageFlags2 pipeline_stage);

        std::weak_ptr<CommandContext> getWeakReference() { return shared_from_this(); }
    private:

        VkCommandPool getCommandPool(Usage usage) const
        {
            switch (usage)
            {
                case CommandContext::Usage::SingleSubmit:
                    return _transient_command_pool;
                case CommandContext::Usage::MultipleSubmit:
                    return _command_pool;
                default:
                    break;
            }
        }

        VkDevice _logical_device{ VK_NULL_HANDLE };
        uint32_t _queue_family_index{ 0 };
        VkQueue _queue{ VK_NULL_HANDLE };
        VkCommandPool _transient_command_pool{ VK_NULL_HANDLE };
        VkCommandPool _command_pool{ VK_NULL_HANDLE };
        DeviceLookup::QueueFamilyInfo _queue_family_info;
    };
}