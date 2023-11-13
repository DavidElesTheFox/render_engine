#include <render_engine/assets/Material.h>

#include <render_engine/assets/Shader.h>
#include <render_engine/assets/Geometry.h>
#include <render_engine/resources/Technique.h>


#include <functional>
#include <ranges>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/Device.h>
#include <set>
#include <numeric>

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
				std::variant<Shader::MetaData::Sampler, Shader::MetaData::UniformBuffer> data;
				TextureView* texture_view{ nullptr };
				VkShaderStageFlags shader_stage{ 0 };
			};

			void addShader(const Shader& shader, VkShaderStageFlags shader_usage)
			{
				for (const auto& [binding, uniform_buffer] : shader.getMetaData().global_uniform_buffers)
				{
					auto it = std::ranges::find_if(_slots, [&](const auto& slot) {return slot.binding == binding; });
					if (it != _slots.end())
					{
						std::visit(overloaded{
							[](const Shader::MetaData::Sampler& sampler) { throw std::runtime_error("Uniform buffer is already bounded as image sampler"); },
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
					auto it = std::ranges::find_if(_slots, [&](const auto& slot) {return slot.binding == binding; });
					if (it != _slots.end())
					{
						std::visit(overloaded{
							[](const Shader::MetaData::Sampler& sampler) {  },
							[&](const Shader::MetaData::UniformBuffer& buffer) { throw std::runtime_error("image sampler is already bounded as Uniform binding"); }
							}, it->data);
						it->shader_stage |= shader_usage;
					}
					BindingSlot data{
						.binding = binding, .data = {sampler}, .shader_stage = shader_usage
					};
					_slots.emplace_back(std::move(data));
				}
			}

			void assignTexture(TextureView* texture_view, int32_t sampler_binding)
			{
				auto it = std::ranges::find_if(_slots, [&](const auto& slot) {return slot.binding == sampler_binding; });
				if (it == _slots.end())
				{
					throw std::runtime_error("There is no image sampler for the given binding");
				}
				if (std::holds_alternative<Shader::MetaData::Sampler>(it->data) == false)
				{
					throw std::runtime_error("Binding is not an image sampler, couldn't bind image for it");
				}
				it->texture_view = texture_view;
			}

			const std::vector<BindingSlot>& getBindings() const { return _slots; }


		private:
			std::vector<BindingSlot> _slots;
		};

		struct UniformCreationData
		{
			BackBuffer<UniformBinding::FrameData> back_buffer;
			int32_t binding{ -1 };
		};
		std::pair<std::vector<UniformBinding>, VkDescriptorSetLayout> createUniformBindings(GpuResourceManager& gpu_resource_manager,
			const MaterialBindingMap& uniform_bindings)
		{
			const auto& uniforms_data = uniform_bindings.getBindings();
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
						[](const Shader::MetaData::Sampler&) {return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; },
						[](const Shader::MetaData::UniformBuffer&) {return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; },
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

			std::vector<VkDescriptorSet> descriptor_sets;
			descriptor_sets.resize(back_buffer_size);
			if (vkAllocateDescriptorSets(gpu_resource_manager.getLogicalDevice(), &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
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

				for (size_t i = 0; i < back_buffer_size; ++i) {
					back_buffer[i].descriptor_set = descriptor_sets[i];

					std::visit(overloaded{
						[&](const Shader::MetaData::Sampler& sampler) 
						{
							back_buffer[i].texture = &buffer_meta_data.texture_view->getTexture();
							{
								VkDescriptorImageInfo image_info{};
								image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
								image_info.imageView = buffer_meta_data.texture_view->getImageView();
								image_info.sampler = buffer_meta_data.texture_view->getSamler();
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


	Material::Material(Shader verted_shader,
		Shader fragment_shader,
		CallbackContainer callbacks,
		uint32_t id)
		: _vertex_shader(verted_shader)
		, _fragment_shader(fragment_shader)
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
		if (_vertex_shader.getMetaData().push_constants.has_value()
			&& _fragment_shader.getMetaData().push_constants.has_value())
		{
			return _vertex_shader.getMetaData().push_constants->size == _fragment_shader.getMetaData().push_constants->size;
		}
		return true;
	}

	

	std::unique_ptr<Technique> MaterialInstance::createTechnique(GpuResourceManager& gpu_resource_manager,
		VkRenderPass render_pass) const
	{
		const Shader& vertex_shader = _material.getVertexShader();
		const Shader& fragment_shader = _material.getFragmentShader();

		MaterialBindingMap binding_map;
		binding_map.addShader(vertex_shader, VK_SHADER_STAGE_VERTEX_BIT);
		binding_map.addShader(fragment_shader, VK_SHADER_STAGE_FRAGMENT_BIT);
		for (auto& [binding, texture] : _texture_bindings)
		{
			binding_map.assignTexture(texture.get(), binding);
		}

		auto&& [bindings, layout] = createUniformBindings(gpu_resource_manager, binding_map);

		return std::make_unique<Technique>(gpu_resource_manager.getLogicalDevice(),
			this,
			std::move(bindings),
			layout,
			render_pass);
	}
}
