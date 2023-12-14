#include "VolumeObject.h"

#include <scene/SceneNodeLookup.h>
namespace Scene
{
    void VolumeObject::onRegisterToNewLookup(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup)
    {
        if (old_lookup != nullptr)
        {
            old_lookup->unregisterVolumeObject(_mesh->getId());
        }
        new_lookup->registerVolumeObject(this, _mesh->getId());
    }
}