#include <render_engine/assets/Material.h>

#include <render_engine/assets/Shader.h>
#include <render_engine/assets/Geometry.h>
#include <render_engine/resources/Technique.h>


#include <functional>
#include <ranges>
#include <render_engine/GpuResourceManager.h>
#include <render_engine/Device.h>


namespace RenderEngine
{
	namespace
	{
		std::pair<std::vector<UniformBinding>, VkDescriptorSetLayout> createUniformBindings(GpuResourceManager& gpu_resource_manager,
			std::ranges::input_range auto&& uniforms_data,
			std::unordered_map<int32_t, VkShaderStageFlags> binding_stage_map,
			VkShaderStageFlags shader_stage)
		{

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

				set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				set_layout_binding.binding = buffer_meta_data.binding;
				set_layout_binding.stageFlags = binding_stage_map[buffer_meta_data.binding];
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
			std::vector<BackBuffer<UniformBinding::FrameData>> back_buffers;

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
			// make buffer assignment
			for (const auto& buffer_meta_data : uniforms_data)
			{
				BackBuffer<UniformBinding::FrameData> back_buffer(back_buffer_size);

				for (size_t i = 0; i < back_buffer_size; ++i) {
					back_buffer[i]._buffer = gpu_resource_manager.createUniformBuffer(buffer_meta_data.size);
					back_buffer[i]._descriptor_set = descriptor_sets[i];

					VkDescriptorBufferInfo buffer_info{};
					buffer_info.buffer = back_buffer[i]._buffer->getBuffer();
					buffer_info.offset = 0;
					buffer_info.range = buffer_meta_data.size;

					VkWriteDescriptorSet writer{};
					writer.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writer.dstSet = descriptor_sets[i];
					writer.dstBinding = buffer_meta_data.binding;
					writer.dstArrayElement = 0;
					writer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					writer.descriptorCount = 1;
					writer.pBufferInfo = &buffer_info;

					descriptor_writers.emplace_back(std::move(writer));
				}
				back_buffers.emplace_back(std::move(back_buffer));
			}
			vkUpdateDescriptorSets(gpu_resource_manager.getLogicalDevice(),
				descriptor_writers.size(), descriptor_writers.data(), 0, nullptr);
			std::vector<UniformBinding> uniform_bindings;
			for (auto& back_buffer : back_buffers)
			{
				uniform_bindings.emplace_back(descriptor_set_layout, std::move(back_buffer), gpu_resource_manager.getLogicalDevice());
			}
			return { std::move(uniform_bindings), descriptor_set_layout };
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
		std::unordered_map<int32_t, VkShaderStageFlags> binding_usage;
		const Shader& vertex_shader = _material->getVertexShader();
		const Shader& fragment_shader = _material->getFragmentShader();
		for (const auto& binding : vertex_shader.getMetaData().global_uniform_buffers | std::views::keys)
		{
			binding_usage[binding] |= VK_SHADER_STAGE_VERTEX_BIT;
		}
		for (const auto& binding : fragment_shader.getMetaData().global_uniform_buffers | std::views::keys)
		{
			binding_usage[binding] |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		auto&& [bindings, layout] = createUniformBindings(gpu_resource_manager,
			std::vector{ vertex_shader.getMetaData().global_uniform_buffers
				| std::views::values, fragment_shader.getMetaData().global_uniform_buffers | std::views::values } | std::views::join,
			binding_usage,
			VK_SHADER_STAGE_VERTEX_BIT);

		return std::make_unique<Technique>(gpu_resource_manager.getLogicalDevice(),
			this,
			std::move(bindings),
			layout,
			render_pass);
	}
}
