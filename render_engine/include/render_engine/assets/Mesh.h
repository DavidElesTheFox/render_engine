#pragma once

#include <cstdint>
#include <vector>

#include <render_engine/assets/Material.h>

namespace RenderEngine
{
	struct Geometry;

	class Mesh
	{
	public:
		Mesh(Geometry* geometry, Material* material, int32_t id)
			: _geometry(std::move(geometry))
			, _material(std::move(material))
			, _id(id)
		{}

		const Geometry& getGeometry() const { return *_geometry; }
		const Material& getMaterial() const { return *_material; }
		int getId() const { return _id; }
		std::vector<uint8_t> createVertexBuffer() const
		{
			return getMaterial().createVertexBufferFromGeometry(getGeometry());
		}
	private:
		Geometry* _geometry{ nullptr };
		Material* _material{ nullptr };
		int32_t _id{ 0 };
	};

	class MeshInstance
	{
	public:
		MeshInstance(Mesh* mesh, MaterialInstance* material_instance, uint32_t id)
			: _mesh(mesh)
			, _material_instance(material_instance)
			, _id(id)
		{}

		Mesh* getMesh() { return _mesh; }
		MaterialInstance* getMaterialInstance() { return _material_instance; }
		void updatePushConstants(PushConstantsUpdater& updater)
		{
			_material_instance->updatePushConstants(this, updater);
		}
		uint32_t getId() const { return _id; }
	private:
		Mesh* _mesh{ nullptr };
		MaterialInstance* _material_instance{ nullptr };
		uint32_t _id{ 0 };
	};
}
