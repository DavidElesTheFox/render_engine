#pragma once
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>

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
            _mesh_instances.insert({ name, std::move(mesh) });
        }

        void addBaseMaterial(std::unique_ptr<IMaterial>&& material)
        {
            _base_materials.insert({ material->getName(), std::move(material) });
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
            return _mesh_instances.at(name).get();
        }
        template<typename T>
        T* getBaseMaterial() const
        {
            return static_cast<T*>(_base_materials.at(T::GetName()).get());
        }

        template<typename T>
        T* getMaterialInstanceAs(const std::string& name) const
        {
            return static_cast<T*>(_material_instances.at(name).get());
        }

        IMaterial::IInstance* getMaterialInstance(const std::string& name) const
        {
            return _material_instances.at(name).get();
        }

        std::ranges::input_range auto getMaterialInstanceNames() const
        {
            return _material_instances | std::views::keys;
        }
        std::ranges::input_range auto getMeshInstanceNames() const
        {
            return _mesh_instances | std::views::keys;
        }
    private:
        std::unordered_map<std::string, std::unique_ptr<RenderEngine::Geometry>> _geometries;
        std::unordered_map<std::string, std::unique_ptr<RenderEngine::Mesh>> _base_meshes;
        std::unordered_map<std::string, std::unique_ptr<RenderEngine::MeshInstance>> _mesh_instances;

        std::unordered_map<std::string, std::unique_ptr<IMaterial>> _base_materials;
        std::unordered_map<std::string, std::unique_ptr<IMaterial::IInstance>> _material_instances;
    };
}