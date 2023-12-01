#include <scene/SceneRenderManager.h>

#include <render_engine/assets/Mesh.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <scene/MeshObject.h>

namespace Scene
{
    void SceneRenderManager::registerMeshesForRender()
    {
        auto& renderer = _window.getRendererAs<RenderEngine::ForwardRenderer>(RenderEngine::ForwardRenderer::kRendererId);
        for (auto mesh : _scene_lookup.getMeshes())
        {
            renderer.addMesh(mesh->getMesh(), mesh->getRenderingPriority());
        }
    }

}