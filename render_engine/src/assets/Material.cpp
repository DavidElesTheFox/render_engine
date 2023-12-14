#include <render_engine/assets/Material.h>

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Shader.h>
#include <render_engine/resources/Technique.h>

#include <render_engine/Device.h>
#include <render_engine/GpuResourceManager.h>


#include <functional>
#include <numeric>
#include <ranges>
#include <set>
#include <variant>

namespace RenderEngine
{
    namespace
    {
        // helper type for the visitor #4
        template<class... Ts>
        struct overloaded : Ts... { using Ts::operator()...; };

        class MaterialBindingMap
        {
        public:
            struct BindingSlot
            {
                int32_t binding{ -1 };
                std::variant<Shader::MetaData::Sampler, Shader::MetaData::UniformBuffer, Shader::MetaData::InputAttachment> data;
                std::vector<ITextureView*> texture_views;
                VkShaderStageFlags shader_stage{ 0 };
            };

            void addShader(const Shader& shader, VkShaderStageFlags shader_usage)
            {
                for (const auto& [binding, uniform_buffer] : shader.getMetaData().global_uniform_buffers)
                {
                    auto it = std::ranges::find_if(_slots, [&](const auto& slot) { return slot.binding == binding; });
                    if (it != _slots.end())
                    {
                        std::visit(overloaded{
                            [](const Shader::MetaData::Sampler& sampler) { throw std::runtime_error("Uniform buffer is already bounded as image sampler"); },
                            [](const Shader::MetaData::InputAttachment&) { throw std::runtime_error("Uniform buffer is already bounded as image sampler"); },
                            [&](const Shader::MetaData::UniformBuffer& buffer) { if (buffer.size != uniform_buffer.size) { throw std::runtime_error("Uniform buffer perviously used a different size"); } }
                                   }, it->data);
                        it->shader_stage |= shader_usage;
                    }
                    BindingSlot data{
                        .binding = binding, .data = {uniform_buffer}, .shader_stage = shader_usage
                    };
                    _slots.emplace_back(std::move(data));
                }
                for (const auto& [binding, sampler] : shader.getMetaData().samplers)
                {
                    auto it = std::ranges::find_if(_slots, [&](const auto& slot) { return slot.binding == binding; });
                    if (it != _slots.end())
                    {
                        std::visit(overloaded{
                            [](const Shader::MetaData::Sampler&) {},
                            [](const Shader::MetaData::UniformBuffer&) { throw std::runtime_error("image sampler is already bounded as Uniform binding"); },
                            [](const Shader::MetaData::InputAttachment&) { throw std::runtime_error("image sampler is already bounded as Uniform binding"); }
                                   }, it->data);
                        it->shader_stage |= shader_usage;
                    }
                    BindingSlot data{
                        .binding = binding, .data = {sampler}, .shader_stage = shader_usage
                    };
                    _slots.emplace_back(std::move(data));
                }
                for (const auto& [binding, input_attachment] : shader.getMetaData().input_attachments)
                {
                    auto it = std::ranges::find_if(_subpass_slots, [&](const auto& slot) { return slot.binding == binding; });
                    if (it != _subpass_slots.end())
                    {
                        std::visit(overloaded{
                            [](const Shader::MetaData::InputAttachment&) {},
                            [](const Shader::MetaData::UniformBuffer&) { throw std::runtime_error("image sampler is already bounded as Uniform binding"); },
                            [](const Shader::MetaData::Sampler&) { throw std::runtime_error("image sampler is already bounded as Uniform binding"); }
                                   }, it->data);
                        it->shader_stage |= shader_usage;
                    }
                    BindingSlot data{
                        .binding = binding, .data = {input_attachment}, .shader_stage = shader_usage
                    };
                    _subpass_slots.emplace_back(std::move(data));
                }
            }

            void assignTexture(const std::vector<ITextureView*> texture_views, int32_t sampler_binding)
            {
                auto it = std::ranges::find_if(_slots, [&](const auto& slot) { return slot.binding == sampler_binding; });
                if (it == _slots.end())
                {
                    throw std::runtime_error("There is no image sampler for the given binding");
                }
                if (std::holds_alternative<Shader::MetaData::Sampler>(it->data) == false
                    && std::holds_alternative<Shader::MetaData::InputAttachment>(it->data) == false)
                {
                    throw std::runtime_error("Binding is not an image sampler, couldn't bind image for it");
                }
                it->texture_views = texture_views;
            }

            void assignTextureToInputAttachment(const std::vector<ITextureView*> texture_views, int32_t sampler_binding)
            {
                auto it = std::ranges::find_if(_subpass_slots, [&](const auto& slot) { return slot.binding == sampler_binding; });
                if (it == _subpass_slots.end())
                {
                    throw std::runtime_error("There is no input attachment for the given binding");
                }
                if (std::holds_alternative<Shader::MetaData::Sampler>(it->data) == false
                    && std::holds_alternative<Shader::MetaData::InputAttachment>(it->data) == false)
                {
                    throw std::runtime_error("Binding is not an image sampler, couldn't bind image for it");
                }
                it->texture_views = texture_views;
            }

            auto getBindings() const { return (std::vector{ _slots, _subpass_slots }) | std::views::join; }


        private:
            std::vector<BindingSlot> _slots;
            std::vector<BindingSlot> _subpass_slots;
        };

        struct UniformCreationData
        {
            BackBuffer<UniformBinding::FrameData> back_buffer;
            int32_t binding{ -1 };
        };
        std::pair<std::vector<UniformBinding>, VkDescriptorSetLayout> createUniformBindings(GpuResourceManager& gpu_resource_manager,
                                                                                            const MaterialBindingMap& uniform_bindings)
        {
            const auto uniforms_data = uniform_bindings.getBindings();
            if (std::ranges::empty(uniforms_data))
            {
                return {};
            }
            uint32_t back_buffer_size = gpu_resource_manager.getBackBufferSize();
            auto descriptor_pool = gpu_resource_manager.getDescriptorPool();
            std::vector<UniformBinding> result;
            // Create resources' layout
            int32_t num_of_bindings = std::distance(uniforms_data.begin(), uniforms_data.end());

            VkDescriptorSetLayoutCreateInfo layout_info{};
            layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layout_info.bindingCount = num_of_bindings;

            std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
            for (const auto& buffer_meta_data : uniforms_data)
            {
                VkDescriptorSetLayoutBinding set_layout_binding = {};

                set_layout_binding.descriptorType =
                    std::visit(overloaded{
                        [](const Shader::MetaData::Sampler&) { return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; },
                        [](const Shader::MetaData::UniformBuffer&) { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; },
                        [](const Shader::MetaData::InputAttachment&) { return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; },
                               }, buffer_meta_data.data);
                set_layout_binding.binding = buffer_meta_data.binding;
                set_layout_binding.stageFlags = buffer_meta_data.shader_stage;
                set_layout_binding.descriptorCount = 1;
                layout_bindings.emplace_back(std::move(set_layout_binding));

            }
            layout_info.pBindings = layout_bindings.data();

            VkDescriptorSetLayout descriptor_set_layout;
            if (vkCreateDescriptorSetLayout(gpu_resource_manager.getLogicalDevice(), &layout_info, nullptr, &descriptor_set_layout)
                != VK_SUCCESS)
            {
                throw std::runtime_error("Cannot crate descriptor set layout for a shader of material");
            }
            // create resources' binding
            std::vector<VkDescriptorSetLayout> layouts(back_buffer_size, descriptor_set_layout);
            std::vector<UniformCreationData> uniform_buffer_creation_data{};

            VkDescriptorSetAllocateInfo alloc_info{};
            alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool = descriptor_pool;
            alloc_info.descriptorSetCount = static_cast<uint32_t>(back_buffer_size);
            alloc_info.pSetLayouts = layouts.data();

            /*
            * TODO create descriptor set not just per back buffers.
            * In meta data create an abstraction where it is possible to share
            * Descriptor sets between different techniques.
            * Also make more general the global UBO and other buffers. Like having an update frequency flag
            * PerObject, PerTechnique, PerFrame, Constant
            *
            * It can be a ShaderResource. And this can be created based on the meta data.
            * Technique is created with these ShaderResources.
            * See more about binding, update frequency and etc here:
            * https://developer.nvidia.com/vulkan-shader-resource-binding
            */
            std::vector<VkDescriptorSet> descriptor_sets;
            descriptor_sets.resize(back_buffer_size);
            if (vkAllocateDescriptorSets(gpu_resource_manager.getLogicalDevice(), &alloc_info, descriptor_sets.data()) != VK_SUCCESS)
            {
                vkDestroyDescriptorSetLayout(gpu_resource_manager.getLogicalDevice(), descriptor_set_layout, nullptr);
                throw std::runtime_error("failed to allocate descriptor sets!");
            }
            std::vector<VkWriteDescriptorSet> descriptor_writers;
            std::list<VkDescriptorImageInfo> image_info_holder;
            std::list<VkDescriptorBufferInfo> buffer_info_holder;
            // make buffer assignment
            for (const auto& buffer_meta_data : uniforms_data)
            {
                BackBuffer<UniformBinding::FrameData> back_buffer(back_buffer_size);

                for (size_t i = 0; i < back_buffer_size; ++i)
                {
                    back_buffer[i].descriptor_set = descriptor_sets[i];

                    std::visit(overloaded{
                        [&](const Shader::MetaData::Sampler& sampler)
                        {
                            back_buffer[i].texture = &buffer_meta_data.texture_views[i]->getTexture();
                            {
                                VkDescriptorImageInfo image_info{};
                                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                image_info.imageView = buffer_meta_data.texture_views[i]->getImageView();
                                image_info.sampler = buffer_meta_data.texture_views[i]->getSamler();
                                image_info_holder.push_back(image_info);
                            }
                            VkWriteDescriptorSet writer{};
                            writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                            writer.dstSet = descriptor_sets[i];
                            writer.dstBinding = buffer_meta_data.binding;
                            writer.dstArrayElement = 0;
                            writer.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                            writer.descriptorCount = 1;
                            writer.pImageInfo = &image_info_holder.back();

                            descriptor_writers.emplace_back(std::move(writer));
                        },
                        [&](const Shader::MetaData::UniformBuffer& buffer)
                        {
                            back_buffer[i].buffer = gpu_resource_manager.createUniformBuffer(buffer.size);
                            {
                                VkDescriptorBufferInfo buffer_info{};
                                buffer_info.buffer = back_buffer[i].buffer->getBuffer();
                                buffer_info.offset = 0;
                                buffer_info.range = buffer.size;
                                buffer_info_holder.push_back(buffer_info);
                            }
                            VkWriteDescriptorSet writer{};
                            writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                            writer.dstSet = descriptor_sets[i];
                            writer.dstBinding = buffer_meta_data.binding;
                            writer.dstArrayElement = 0;
                            writer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                            writer.descriptorCount = 1;
                            writer.pBufferInfo = &buffer_info_holder.back();

                            descriptor_writers.emplace_back(std::move(writer));
                        },
                        [&](const Shader::MetaData::InputAttachment& input_attachment)
                        {
                            back_buffer[i].texture = &buffer_meta_data.texture_views[i]->getTexture();
                            {
                                VkDescriptorImageInfo image_info{};
                                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                image_info.imageView = buffer_meta_data.texture_views[i]->getImageView();
                                image_info.sampler = buffer_meta_data.texture_views[i]->getSamler();
                                image_info_holder.push_back(image_info);
                            }
                            VkWriteDescriptorSet writer{};
                            writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                            writer.dstSet = descriptor_sets[i];
                            writer.dstBinding = buffer_meta_data.binding;
                            writer.dstArrayElement = 0;
                            writer.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                            writer.descriptorCount = 1;
                            writer.pImageInfo = &image_info_holder.back();

                            descriptor_writers.emplace_back(std::move(writer));
                        }
                               }, buffer_meta_data.data);
                }
                uniform_buffer_creation_data.emplace_back(std::move(back_buffer), buffer_meta_data.binding);
            }
            vkUpdateDescriptorSets(gpu_resource_manager.getLogicalDevice(),
                                   descriptor_writers.size(), descriptor_writers.data(), 0, nullptr);
            for (auto& [back_buffer, binding] : uniform_buffer_creation_data)
            {
                result.emplace_back(descriptor_set_layout,
                                    std::move(back_buffer),
                                    binding,
                                    gpu_resource_manager.getLogicalDevice());
            }
            return { std::move(result), descriptor_set_layout };
        }
    }


    Material::Material(std::unique_ptr<Shader> verted_shader,
                       std::unique_ptr<Shader> fragment_shader,
                       CallbackContainer callbacks,
                       uint32_t id)
        : _vertex_shader(std::move(verted_shader))
        , _fragment_shader(std::move(fragment_shader))
        , _id{ id }
        , _callbacks(std::move(callbacks))
    {
        if (checkPushConstantsConsistency() == false)
        {
            //	throw std::runtime_error("Materials push constants are not consistant across the shaders.");
        }
    }

    bool Material::checkPushConstantsConsistency() const
    {
        if (_vertex_shader->getMetaData().push_constants.has_value()
            && _fragment_shader->getMetaData().push_constants.has_value())
        {
            return _vertex_shader->getMetaData().push_constants->size == _fragment_shader->getMetaData().push_constants->size;
        }
        return true;
    }

    std::unique_ptr<Technique> MaterialInstance::createTechnique(GpuResourceManager& gpu_resource_manager,
                                                                 TextureBindingData&& subpass_textures,
                                                                 VkRenderPass render_pass,
                                                                 uint32_t corresponding_subpass) const
    {
        const Shader& vertex_shader = _material.getVertexShader();
        const Shader& fragment_shader = _material.getFragmentShader();

        MaterialBindingMap binding_map;
        binding_map.addShader(vertex_shader, VK_SHADER_STAGE_VERTEX_BIT);
        binding_map.addShader(fragment_shader, VK_SHADER_STAGE_FRAGMENT_BIT);
        auto texture_views_per_binding = _texture_bindings.createTextureViews(gpu_resource_manager.getBackBufferSize());
        for (auto& [binding, texture_views] : texture_views_per_binding)
        {
            binding_map.assignTexture(texture_views, binding);
        }
        auto subpass_texture_views = subpass_textures.createTextureViews(gpu_resource_manager.getBackBufferSize());
        for (auto& [binding, texture_views] : subpass_texture_views)
        {
            binding_map.assignTextureToInputAttachment(texture_views, binding);
        }
        auto&& [bindings, layout] = createUniformBindings(gpu_resource_manager, binding_map);

        return std::make_unique<Technique>(gpu_resource_manager.getLogicalDevice(),
                                           this,
                                           std::move(subpass_textures),
                                           std::move(bindings),
                                           layout,
                                           render_pass,
                                           corresponding_subpass);
    }

    std::unordered_map<int32_t, std::vector<ITextureView*>> MaterialInstance::TextureBindingData::createTextureViews(int32_t back_buffer_size) const
    {
        std::unordered_map<int32_t, std::vector<ITextureView*>> result;
        assert(_general_texture_bindings.empty() || _back_buffered_texture_bindings.empty()
               && "Textures are assigned per frame buffers or general. Mixture is not allowed");
        for (auto& [binding, texture_view] : _general_texture_bindings)
        {
            result[binding] = std::vector(back_buffer_size, texture_view.get());
        }
        for (auto& [binding, texture_views] : _back_buffered_texture_bindings)
        {
            std::ranges::transform(texture_views, std::back_inserter(result[binding]),
                                   [](auto& ptr) { return ptr.get(); });
        }
        return result;
    }

    MaterialInstance::TextureBindingData MaterialInstance::cloneBindings() const
    {
        return _texture_bindings.clone();
    }

    MaterialInstance::TextureBindingData MaterialInstance::TextureBindingData::clone() const
    {
        TextureBindingData clone;
        for (auto& [binding, texture_view] : _general_texture_bindings)
        {
            clone._general_texture_bindings[binding] = texture_view->clone();
        }
        for (auto& [binding, texture_views] : _back_buffered_texture_bindings)
        {
            for (const auto& texture_view : texture_views)
            {
                clone._back_buffered_texture_bindings[binding].push_back(texture_view->clone());
            }
        }
        return clone;
    }
}
