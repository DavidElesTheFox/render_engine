#pragma once

#include <render_engine/assets/Geometry.h>
#include <render_engine/assets/Mesh.h>
#include <render_engine/assets/VolumeMaterial.h>
#include <render_engine/assets/VolumeShader.h>

#include <glm/vec3.hpp>


namespace RenderEngine
{
    class VolumetricObject : public Mesh
    {
    public:
        static std::unique_ptr<VolumetricObject> create(const glm::vec3& extent, VolumeMaterial* material, int32_t id)
        {
            return std::unique_ptr<VolumetricObject>(new VolumetricObject(generateGeometry(extent), material, id));
        }

    private:
        explicit VolumetricObject(std::unique_ptr<Geometry> geometry, VolumeMaterial* material, int32_t id)
            : Mesh(geometry.get(), material, id)
            , _bounding_box_geometry(std::move(geometry))
        {}
        static std::unique_ptr<Geometry> generateGeometry(const glm::vec3& extent);

        std::unique_ptr<Geometry> _bounding_box_geometry;
    };

    class VolumetricObjectInstance : public MeshInstance
    {
    public:
        VolumetricObjectInstance(VolumetricObject* mesh, VolumeMaterialInstance* material_instance, uint32_t id)
            : MeshInstance(mesh, material_instance, id)
        {}
        ~VolumetricObjectInstance() override = default;

        const VolumeMaterialInstance* getVolumeMaterialInstance() const { return static_cast<const VolumeMaterialInstance*>(getMaterialInstance()); }
    };
}