#include <scene/SceneNodeLookup.h>

namespace Scene
{
	MeshObject* SceneNodeLookup::findMesh(uint32_t id) const
	{
		if (auto it = _meshes.find(id); it != _meshes.end())
		{
			return it->second;
		}
		return nullptr;
	}
	Camera* SceneNodeLookup::findCamera(const std::string& name) const
	{
		if (auto it = _cameras.find(name); it != _cameras.end())
		{
			return it->second;
		}
		return nullptr;
	}
}