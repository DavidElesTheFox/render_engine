#pragma once

#include <render_engine/assets/Shader.h>


#include <volk.h>

#include <functional>
#include <memory>
#include <vector>

namespace RenderEngine
{
	class GpuResourceManager;
	class Technique;
	class UniformBinding;
	struct Geometry;

	class Material
	{
	public:
		Material(Shader verted_shader,
			Shader fragment_shader,
			std::function<void(std::vector<UniformBinding>& ubo_container, uint32_t frame_number)> buffer_update_callback,
			std::function<std::vector<uint8_t>(const Geometry& geometry, const Material& material)> vertex_buffer_create_callback,
			uint32_t id);
		const Shader& vertexShader() const { return _vertex_shader; }
		const Shader& fragmentShader() const { return _fragment_shader; }

		std::unique_ptr<Technique> createTechnique(VkDevice logical_device,
			GpuResourceManager& gpu_resource_manager,
			VkRenderPass render_pass) const;

		uint32_t getId() const { return _id; }

		std::vector<uint8_t> createVertexBufferFromGeometry(const Geometry& geometry) const
		{
			return _vertex_buffer_create_callback(geometry, *this);
		}
		void updateUniformBuffers(std::vector<UniformBinding>& ubo_container, uint32_t frame_number) const
		{
			_buffer_update_callback(ubo_container, frame_number);
		}
	private:
		std::pair<std::vector<UniformBinding>, VkDescriptorSetLayout> createUniformBindings(GpuResourceManager& gpu_resource_manager,
			std::ranges::input_range auto&& uniforms_data,
			std::unordered_map<int32_t, VkShaderStageFlags> binding_stage_map,
			VkShaderStageFlags shader_stage) const;
		Shader _vertex_shader;
		Shader _fragment_shader;
		uint32_t _id;
		std::function<void(std::vector<UniformBinding>& ubo_container, uint32_t frame_number)> _buffer_update_callback;
		std::function<std::vector<uint8_t>(const Geometry& geometry, const Material& material)> _vertex_buffer_create_callback;
	};
}
