#pragma once

#include <scene/SceneObject.h>
#include <render_engine/assets/Mesh.h>
namespace Scene
{ 

	class MeshObject : public SceneObject
	{
	public:
		MeshObject(std::string name, RenderEngine::MeshInstance* mesh, int32_t rendering_priority = 0)
			: SceneObject(std::move(name))
			, _mesh(mesh)
			, _rendering_priority(rendering_priority)
		{

		}
		~MeshObject() override = default;

		const RenderEngine::MeshInstance* getMesh() const { return _mesh; }

		void onRegisterToNewLookup(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup) override;

		int32_t getRenderingPriority() const { return _rendering_priority; }
	private:
		RenderEngine::MeshInstance* _mesh{ nullptr };
		int32_t _rendering_priority{ 0 };
	};
}