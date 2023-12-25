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
            auto* renderer = static_cast<RenderEngine::ForwardRenderer*>(_window.findRenderer(RenderEngine::ForwardRenderer::kRendererId));
            if (_scene_lookup.getMeshes().empty() == false && renderer == nullptr)
            {
                throw std::runtime_error("Couldn't find renderer to register meshes");
            }
            for (auto mesh : _scene_lookup.getMeshes())
            {
                renderer->addMesh(mesh->getMesh());
            }
        }
        {
            auto* renderer = static_cast<RenderEngine::VolumeRenderer*>(_window.findRenderer(RenderEngine::VolumeRenderer::kRendererId));
            if (_scene_lookup.getVolumetricObject().empty() == false && renderer == nullptr)
            {
                throw std::runtime_error("Couldn't find renderer to register volumetric meshes");
            }
            for (auto mesh : _scene_lookup.getVolumetricObject())
            {
                renderer->addVolumeObject(static_cast<const RenderEngine::VolumetricObjectInstance*>(mesh->getMesh()));
            }
        }
    }

}