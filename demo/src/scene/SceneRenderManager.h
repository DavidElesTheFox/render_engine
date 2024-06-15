#pragma once

#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/VolumeRenderer.h>
#include <render_engine/window/Window.h>
#include <render_engine/window/Window.h>
#include <scene/SceneNodeLookup.h>

namespace Scene
{
    class SceneRenderManager
    {
    public:
        SceneRenderManager(const SceneNodeLookup& scene_lookup,
                           RenderEngine::ForwardRenderer* forward_renderer,
                           RenderEngine::VolumeRenderer* volume_renderer)
            : _scene_lookup(scene_lookup)
            , _forward_renderer(forward_renderer)
            , _volume_renderer(volume_renderer)
        {}

        void registerMeshesForRender();
    private:
        const SceneNodeLookup& _scene_lookup;
        RenderEngine::ForwardRenderer* _forward_renderer{ nullptr };
        RenderEngine::VolumeRenderer* _volume_renderer{ nullptr };
    };
}