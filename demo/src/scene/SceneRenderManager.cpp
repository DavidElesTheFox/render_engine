#include <scene/SceneRenderManager.h>

#include <render_engine/assets/Mesh.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/VolumeRenderer.h>

#include <scene/MeshObject.h>
#include <scene/VolumeObject.h>

namespace Scene
{
    void SceneRenderManager::registerMeshesForRender()
    {
        {
            if (_scene_lookup.getMeshes().empty() == false && _forward_renderer == nullptr)
            {
                throw std::runtime_error("Couldn't find renderer to register meshes");
            }
            for (auto mesh : _scene_lookup.getMeshes())
            {
                _forward_renderer->addMesh(mesh->getMesh());
            }
        }
        {
            if (_scene_lookup.getVolumetricObject().empty() == false && _volume_renderer == nullptr)
            {
                throw std::runtime_error("Couldn't find renderer to register volumetric meshes");
            }
            for (auto mesh : _scene_lookup.getVolumetricObject())
            {
                _volume_renderer->addVolumeObject(static_cast<const RenderEngine::VolumetricObjectInstance*>(mesh->getMesh()));
            }
        }
    }

}