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
	class PushConstantsUpdater;
	class Mesh;
	struct Geometry;

	class Material
	{
	public:
		struct CallbackContainer
		{
			std::function<void(std::vector<UniformBinding>& ubo_container, uint32_t frame_number)> buffer_update_callback;
			std::function<std::vector<uint8_t>(const Geometry& geometry, const Material& material)> vertex_buffer_create_callback;
			std::function<void(Mesh* mesh, PushConstantsUpdater& updater)> push_constants_updater;
			std::function<void(PushConstantsUpdater& updater)> push_constants_global_updater;
		};
		Material(Shader verted_shader,
			Shader fragment_shader,
			CallbackContainer callbacks,
			uint32_t id);
		const Shader& getVertexShader() const { return _vertex_shader; }
		const Shader& getFragmentShader() const { return _fragment_shader; }

		std::unique_ptr<Technique> createTechnique(VkDevice logical_device,
			GpuResourceManager& gpu_resource_manager,
			VkRenderPass render_pass) const;

		uint32_t getId() const { return _id; }

		std::vector<uint8_t> createVertexBufferFromGeometry(const Geometry& geometry) const
		{
			return _callbacks.vertex_buffer_create_callback(geometry, *this);
		}
		void updateUniformBuffers(std::vector<UniformBinding>& ubo_container, uint32_t frame_number) const
		{
			_callbacks.buffer_update_callback(ubo_container, frame_number);
		}

		void updatePushConstants(Mesh* mesh, PushConstantsUpdater& updater) const
		{
			_callbacks.push_constants_updater(mesh, updater);
		}

		void updatePushConstants(PushConstantsUpdater& updater) const
		{
			_callbacks.push_constants_global_updater(updater);
		}

		const std::optional<Shader::MetaData::PushConstants>& getPushConstants() const
		{
			return _vertex_shader.getMetaData().push_constants != std::nullopt
				? _vertex_shader.getMetaData().push_constants
				: _fragment_shader.getMetaData().push_constants;
		}
	private:

		bool checkPushConstantsConsistency() const;
		std::pair<std::vector<UniformBinding>, VkDescriptorSetLayout> createUniformBindings(GpuResourceManager& gpu_resource_manager,
			std::ranges::input_range auto&& uniforms_data,
			std::unordered_map<int32_t, VkShaderStageFlags> binding_stage_map,
			VkShaderStageFlags shader_stage) const;
		Shader _vertex_shader;
		Shader _fragment_shader;
		uint32_t _id;
		CallbackContainer _callbacks;
	};
}
