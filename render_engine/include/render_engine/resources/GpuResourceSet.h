#pragma once

#include <volk.h>

#include <render_engine/assets/TextureAssignment.h>
#include <render_engine/containers/Views.h>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/resources/UniformBinding.h>
namespace RenderEngine
{
    class GpuResourceSet
    {
    public:
        GpuResourceSet(GpuResourceManager& resource_manager,
                       const std::vector<TextureAssignment::BindingSlot>& texture_binding);

        ~GpuResourceSet();

        GpuResourceSet(GpuResourceSet&& o) noexcept
            : _resources(std::move(o._resources))
            , _resource_layout(std::move(o._resource_layout))
            , _logical_device(std::move(o._logical_device))
        {
            o._resource_layout = VK_NULL_HANDLE;
            o._logical_device = VK_NULL_HANDLE;
        }
        GpuResourceSet& operator=(GpuResourceSet&& o)
        {
            using std::swap;
            swap(_resources, o._resources);
            swap(_resource_layout, o._resource_layout);
            swap(_logical_device, o._logical_device);
        }

        GpuResourceSet(const GpuResourceSet&) = delete;
        GpuResourceSet& operator=(const GpuResourceSet&) = delete;

        std::vector<UniformBinding*> getResources() const
        {
            auto range = _resources | views::to_raw_pointer;
            return { range.begin(), range.end() };
        }
        VkDescriptorSetLayout getLayout() const { return _resource_layout; }
    private:
        LogicalDevice& getLogicalDevice() { return *_logical_device; }

        std::vector<std::unique_ptr<UniformBinding>> _resources;
        VkDescriptorSetLayout _resource_layout{ VK_NULL_HANDLE };
        LogicalDevice* _logical_device{ nullptr };
    };
}