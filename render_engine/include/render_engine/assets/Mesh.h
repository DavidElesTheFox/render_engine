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
}
