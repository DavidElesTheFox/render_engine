#include <scene/MeshObject.h>

#include <scene/SceneNodeLookup.h>
namespace Scene
{
    void MeshObject::onRegisterToNewLookup(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup)
    {
        if (old_lookup != nullptr)
        {
            old_lookup->unregisterMesh(_mesh->getId());
        }
        new_lookup->registerMesh(this, _mesh->getId());
    }
}