#include <render_engine/resources/GpuResourceSet.h>

#include <render_engine/containers/VariantOverloaded.h>
namespace RenderEngine
{
    namespace
    {
        struct UniformCreationData
        {
            BackBuffer<UniformBinding::FrameData> back_buffer;
            int32_t binding{ -1 };
        };
    }
    GpuResourceSet::GpuResourceSet(GpuResourceManager& gpu_resource_manager,
                                   const std::vector<TextureAssignment::BindingSlot>& binding_slots)
        : _logical_device(&gpu_resource_manager.getLogicalDevice())
    {
        if (binding_slots.empty())
        {
            return;
        }
        uint32_t back_buffer_size = gpu_resource_manager.getBackBufferSize();
        auto descriptor_pool = gpu_resource_manager.getDescriptorPool();
        // Create resources' layout
        const int32_t num_of_bindings = binding_slots.size();

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = num_of_bindings;

        std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
        for (const TextureAssignment::BindingSlot& slot : binding_slots)
        {
            VkDescriptorSetLayoutBinding set_layout_binding = {};

            set_layout_binding.descriptorType =
                std::visit(overloaded{
                    [](const Shader::MetaData::Sampler&) { return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; },
                    [](const Shader::MetaData::UniformBuffer&) { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; },
                    [](const Shader::MetaData::InputAttachment&) { return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; },
                           }, slot.data);
            set_layout_binding.binding = slot.binding;
            set_layout_binding.stageFlags = slot.shader_stage;
            set_layout_binding.descriptorCount = 1;
            layout_bindings.emplace_back(std::move(set_layout_binding));

        }
        layout_info.pBindings = layout_bindings.data();

        if (getLogicalDevice()->vkCreateDescriptorSetLayout(*getLogicalDevice(), &layout_info, nullptr, &_resource_layout)
            != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot crate descriptor set layout for a shader of material");
        }

        // create resources' binding
        std::vector<VkDescriptorSetLayout> layouts(back_buffer_size, _resource_layout);
        std::vector<UniformCreationData> uniform_buffer_creation_data{};

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = descriptor_pool;
        alloc_info.descriptorSetCount = static_cast<uint32_t>(back_buffer_size);
        alloc_info.pSetLayouts = layouts.data();

        std::vector<VkDescriptorSet> descriptor_sets;
        descriptor_sets.resize(back_buffer_size);
        if (getLogicalDevice()->vkAllocateDescriptorSets(*getLogicalDevice(), &alloc_info, descriptor_sets.data()) != VK_SUCCESS)
        {
            getLogicalDevice()->vkDestroyDescriptorSetLayout(*getLogicalDevice(), _resource_layout, nullptr);
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
        std::vector<VkWriteDescriptorSet> descriptor_writers;
        std::list<VkDescriptorImageInfo> image_info_holder;
        std::list<VkDescriptorBufferInfo> buffer_info_holder;
        // make buffer assignment
        for (const TextureAssignment::BindingSlot& slot : binding_slots)
        {
            BackBuffer<UniformBinding::FrameData> back_buffer(back_buffer_size);

            for (size_t i = 0; i < back_buffer_size; ++i)
            {
                back_buffer[i].descriptor_set = descriptor_sets[i];

                std::visit(overloaded{
                    [&](const Shader::MetaData::Sampler& sampler)
                    {
                        back_buffer[i].texture = &slot.texture_views[i]->getTexture();
                        {
                            VkDescriptorImageInfo image_info{};
                            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            image_info.imageView = slot.texture_views[i]->getImageView();
                            image_info.sampler = slot.texture_views[i]->getSampler();
                            image_info_holder.push_back(image_info);
                        }
                        VkWriteDescriptorSet writer{};
                        writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writer.dstSet = descriptor_sets[i];
                        writer.dstBinding = slot.binding;
                        writer.dstArrayElement = 0;
                        writer.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        writer.descriptorCount = 1;
                        writer.pImageInfo = &image_info_holder.back();

                        descriptor_writers.emplace_back(std::move(writer));
                    },
                    [&](const Shader::MetaData::UniformBuffer& buffer)
                    {
                        back_buffer[i].coherent_buffer = gpu_resource_manager.createUniformBuffer(buffer.size);
                        {
                            VkDescriptorBufferInfo buffer_info{};
                            buffer_info.buffer = back_buffer[i].coherent_buffer->getBuffer();
                            buffer_info.offset = 0;
                            buffer_info.range = buffer.size;
                            buffer_info_holder.push_back(buffer_info);
                        }
                        VkWriteDescriptorSet writer{};
                        writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writer.dstSet = descriptor_sets[i];
                        writer.dstBinding = slot.binding;
                        writer.dstArrayElement = 0;
                        writer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        writer.descriptorCount = 1;
                        writer.pBufferInfo = &buffer_info_holder.back();

                        descriptor_writers.emplace_back(std::move(writer));
                    },
                    [&](const Shader::MetaData::InputAttachment& input_attachment)
                    {
                        back_buffer[i].texture = &slot.texture_views[i]->getTexture();
                        {
                            VkDescriptorImageInfo image_info{};
                            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            image_info.imageView = slot.texture_views[i]->getImageView();
                            image_info.sampler = slot.texture_views[i]->getSampler();
                            image_info_holder.push_back(image_info);
                        }
                        VkWriteDescriptorSet writer{};
                        writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writer.dstSet = descriptor_sets[i];
                        writer.dstBinding = slot.binding;
                        writer.dstArrayElement = 0;
                        writer.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                        writer.descriptorCount = 1;
                        writer.pImageInfo = &image_info_holder.back();

                        descriptor_writers.emplace_back(std::move(writer));
                    }
                           }, slot.data);
            }
            uniform_buffer_creation_data.emplace_back(std::move(back_buffer), slot.binding);
        }
        getLogicalDevice()->vkUpdateDescriptorSets(*getLogicalDevice(),
                                                   descriptor_writers.size(), descriptor_writers.data(), 0, nullptr);
        for (auto& [back_buffer, binding] : uniform_buffer_creation_data)
        {
            _resources.emplace_back(std::make_unique<UniformBinding>(_resource_layout,
                                                                     std::move(back_buffer),
                                                                     binding,
                                                                     gpu_resource_manager.getLogicalDevice()));
        }
    }
    GpuResourceSet::~GpuResourceSet()
    {
        if (_logical_device != nullptr)
        {
            getLogicalDevice()->vkDestroyDescriptorSetLayout(*getLogicalDevice(), _resource_layout, nullptr);
        }
    }
}