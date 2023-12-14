#pragma once
#include <scene/SceneObject.h>

#include <render_engine/assets/VolumetricObject.h>
namespace Scene
{
    class VolumeObject : public SceneObject
    {
    public:
        VolumeObject(std::string name, RenderEngine::VolumetricObjectInstance* mesh)
            : SceneObject(std::move(name))
            , _mesh(mesh)
        {

        }
        ~VolumeObject() override = default;

        const RenderEngine::MeshInstance* getMesh() const { return _mesh; }

        void onRegisterToNewLookup(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup) override;

    private:
        RenderEngine::VolumetricObjectInstance* _mesh{ nullptr };
    };
}