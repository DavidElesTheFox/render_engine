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
	class MeshInstance;
	struct Geometry;

	class Material
	{
	public:
		struct CallbackContainer
		{
			std::function<std::vector<uint8_t>(const Geometry& geometry, const Material& material)> create_vertex_buffer;
		};
		Material(Shader verted_shader,
			Shader fragment_shader,
			CallbackContainer callbacks,
			uint32_t id);
		const Shader& getVertexShader() const { return _vertex_shader; }
		const Shader& getFragmentShader() const { return _fragment_shader; }

		uint32_t getId() const { return _id; }

		std::vector<uint8_t> createVertexBufferFromGeometry(const Geometry& geometry) const
		{
			return _callbacks.create_vertex_buffer(geometry, *this);
		}

		std::unordered_map<VkShaderStageFlags, Shader::MetaData::PushConstants> getPushConstantsMetaData() const
		{
			std::unordered_map<VkShaderStageFlags, Shader::MetaData::PushConstants> result;
			if (_vertex_shader.getMetaData().push_constants != std::nullopt)
			{
				result.insert({ VK_SHADER_STAGE_VERTEX_BIT | _vertex_shader.getMetaData().push_constants->shared_with,*_vertex_shader.getMetaData().push_constants });
			}
			if (_fragment_shader.getMetaData().push_constants != std::nullopt)
			{
				result.insert({ VK_SHADER_STAGE_FRAGMENT_BIT | _fragment_shader.getMetaData().push_constants->shared_with, *_fragment_shader.getMetaData().push_constants });
			}
			return result;
		}
	private:

		bool checkPushConstantsConsistency() const;

		Shader _vertex_shader;
		Shader _fragment_shader;
		uint32_t _id;
		CallbackContainer _callbacks;
	};

	class MaterialInstance
	{
	public:
		struct CallbackContainer
		{
			std::function<void(std::vector<UniformBinding>& ubo_container, uint32_t frame_number)> global_ubo_update;
			std::function<void(PushConstantsUpdater& updater)> global_push_constants_update;
			std::function<void(const MeshInstance* mesh_instance, PushConstantsUpdater& updater)> push_constants_updater;
		};
		MaterialInstance(Material* material, CallbackContainer callbacks, uint32_t id)
			: _material(material)
			, _callbacks(std::move(callbacks))
			, _id(id)
		{}

		std::unique_ptr<Technique> createTechnique(GpuResourceManager& gpu_resource_manager,
			VkRenderPass render_pass) const;

		void updateGlobalUniformBuffer(std::vector<UniformBinding>& ubo_container, uint32_t frame_number) const
		{
			_callbacks.global_ubo_update(ubo_container, frame_number);
		}

		void updateGlobalPushConstants(PushConstantsUpdater& updater) const
		{
			_callbacks.global_push_constants_update(updater);
		}
		void updatePushConstants(const MeshInstance* mesh_instance, PushConstantsUpdater& updater) const
		{
			_callbacks.push_constants_updater(mesh_instance, updater);
		}
		uint32_t getId() const { return _id; }

		const Material* getMaterial() const { return _material; }
	private:


		Material* _material{ nullptr };
		CallbackContainer _callbacks;
		uint32_t _id{ 0 };
	};
}
