#pragma once
#include <render_engine/resources/Texture.h>

#include <assets/AssetDatabase.h>
#include <scene/Scene.h>

#include <memory>

class DemoSceneBuilder
{
public:
    struct CreationResult
    {
        CreationResult() = default;
        CreationResult(const CreationResult&) = delete;
        CreationResult& operator=(const CreationResult&) = delete;
        CreationResult(CreationResult&&) = default;
        CreationResult& operator=(CreationResult&&) = default;

        std::vector<std::unique_ptr<RenderEngine::Texture>> textures;
    };
    CreationResult buildSceneOfQuads(Assets::AssetDatabase& assets,
                                     Scene::Scene& scene,
                                     RenderEngine::TextureFactory& texture_factory,
                                     RenderEngine::RenderEngine& render_engine);
};
