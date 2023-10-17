#pragma once
#include <unordered_map>
#include <string>
#include <memory>

#include <assets/IMaterial.h>

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Mesh.h>
namespace Assets
{
	class AssetDatabase
	{
	public:

		void addGeometry(const std::string& name, std::unique_ptr<RenderEngine::Geometry>&& geometry)
		{
			_geometries.insert({ name, std::move(geometry) });
		}

		void addBaseMesh(const std::string& name, std::unique_ptr<RenderEngine::Mesh>&& mesh)
		{
			_base_meshes.insert({ name, std::move(mesh) });
		}
		void addMeshInstance(const std::string& name, std::unique_ptr<RenderEngine::MeshInstance>&& mesh)
		{
			_meshes_instances.insert({ name, std::move(mesh) });
		}

		void addBaseMaterial(const std::string& name, std::unique_ptr<IMaterial>&& material)
		{
			_base_materials.insert({ name, std::move(material) });
		}

		void addMaterialInstance(const std::string& name, std::unique_ptr<IMaterial::IInstance>&& material_instance)
		{
			_material_instances.insert({ name, std::move(material_instance) });
		}

		RenderEngine::Geometry* getGeometry(const std::string& name) const
		{
			return _geometries.at(name).get();
		}

		RenderEngine::Mesh* getBaseMesh(const std::string& name) const
		{
			return _base_meshes.at(name).get();
		}

		RenderEngine::MeshInstance* getMeshInstance(const std::string& name) const
		{
			return _meshes_instances.at(name).get();
		}
		template<typename T>
		T* getBaseMaterialAs(const std::string& name) const
		{
			return static_cast<T*>(_base_materials.at(name).get());
		}

		template<typename T>
		T* getMaterialInstanceAs(const std::string& name) const
		{
			return static_cast<T*>(_material_instances.at(name).get());
		}
	private:
		std::unordered_map<std::string, std::unique_ptr<RenderEngine::Geometry>> _geometries;
		std::unordered_map<std::string, std::unique_ptr<RenderEngine::Mesh>> _base_meshes;
		std::unordered_map<std::string, std::unique_ptr<RenderEngine::MeshInstance>> _meshes_instances;

		std::unordered_map<std::string, std::unique_ptr<IMaterial>> _base_materials;
		std::unordered_map<std::string, std::unique_ptr<IMaterial::IInstance>> _material_instances;
	};
}