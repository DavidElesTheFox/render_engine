#pragma once

#include <unordered_map>
#include <string>
#include <ranges>
namespace Scene
{
	class MeshObject;
	class Camera;
	class SceneNodeLookup
	{
	public:
		void registerMesh(MeshObject* object, uint32_t id)
		{
			_meshes.insert({ id, object });
		}
		void registerCamera(Camera* camera, const std::string& name)
		{
			_cameras.insert({ name, camera });
		}
		void unregisterMesh(uint32_t id)
		{
			_meshes.erase(id);
		}
		void unregisterCamera(const std::string& name)
		{
			_cameras.erase(name);
		}
		MeshObject* findMesh(uint32_t id) const;
		Camera* findCamera(const std::string& name) const;

		std::ranges::input_range auto getMeshes() const { return _meshes | std::views::values; }
		std::ranges::input_range auto getCameras() const { return _cameras | std::views::values; }
	private:
		std::unordered_map<uint32_t, MeshObject*> _meshes;
		std::unordered_map<std::string, Camera*> _cameras;

	};
}