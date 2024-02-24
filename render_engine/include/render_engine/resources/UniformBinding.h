#pragma once

#include <volk.h>

#include <memory>

#include <render_engine/containers/BackBuffer.h>
#include <render_engine/resources/Buffer.h>
#include <render_engine/resources/Texture.h>

namespace RenderEngine
{

    class UniformBinding
    {
    public:
        struct FrameData
        {
            VkDescriptorSet descriptor_set{ VK_NULL_HANDLE };
            std::unique_ptr<CoherentBuffer> coherent_buffer{};
            Texture* texture{ nullptr };
        };

        UniformBinding(BackBuffer<FrameData>&& back_buffer,
                       int32_t binding,
                       LogicalDevice& logical_device)
            : _logical_device(logical_device)
            , _back_buffer(std::move(back_buffer))
            , _binding(binding)
        {}
        ~UniformBinding() = default;
        UniformBinding(const UniformBinding&) = delete;
        UniformBinding(UniformBinding&&) = default;
        UniformBinding& operator=(const UniformBinding&) = delete;
        UniformBinding& operator=(UniformBinding&&) = default;

        CoherentBuffer& getBuffer(size_t frame_number) const
        {
            return *_back_buffer[frame_number].coherent_buffer;
        }
        VkDescriptorSet getDescriptorSet(size_t frame_number) const
        {
            return _back_buffer[frame_number].descriptor_set;
        }
        Texture* getTextureForFrame(size_t frame_number) const
        {
            return _back_buffer[frame_number].texture;
        }
    private:
        LogicalDevice& _logical_device;
        BackBuffer<FrameData> _back_buffer;
        int32_t _binding{ -1 };
    };
}
