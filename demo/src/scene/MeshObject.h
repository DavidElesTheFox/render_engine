#pragma once

#include <scene/SceneObject.h>
#include <render_engine/assets/Mesh.h>
namespace Scene
{ 

	class MeshObject : public SceneObject
	{
	public:
		MeshObject(std::string name, RenderEngine::MeshInstance* mesh)
			: SceneObject(std::move(name))
			, _mesh(mesh)
		{

		}
		~MeshObject() override = default;

		const RenderEngine::MeshInstance* getMesh() const { return _mesh; }

		void onRegisterToNewLookup(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup) override;
	private:
		RenderEngine::MeshInstance* _mesh{ nullptr };
	};
}