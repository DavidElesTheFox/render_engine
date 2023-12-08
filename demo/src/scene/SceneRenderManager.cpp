#include <scene/SceneRenderManager.h>

#include <render_engine/assets/Mesh.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <scene/MeshObject.h>

namespace Scene
{
    void SceneRenderManager::registerMeshesForRender()
    {
        auto* renderer = static_cast<RenderEngine::ForwardRenderer*>(_window.findRenderer(RenderEngine::ForwardRenderer::kRendererId));
        if (renderer == nullptr)
        {
            throw std::runtime_error("Couldn't find renderer to register meshes");
        }
        for (auto mesh : _scene_lookup.getMeshes())
        {
            renderer->addMesh(mesh->getMesh());
        }
    }

}