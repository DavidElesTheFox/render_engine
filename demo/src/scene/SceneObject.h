#pragma once

#include <scene/Transformation.h>
#include <string>

namespace Scene
{
	class SceneNode;
	class Scene;
	class SceneNodeLookup;
	class SceneObject
	{
	public:
		SceneObject(std::string name)
			: _name(std::move(name))
		{};
		virtual ~SceneObject() = 0 {};

		const Transformation& getTransformation() const { return _transformation; }
		Transformation& getTransformation() { return _transformation; }
		const std::string& getName() const { return _name; }
		
		void changeParent(SceneNode* parent)
		{
			std::swap(parent, _parent);
			onParentChanged(_parent, parent);
		}
		const SceneNode* getParent() const { return _parent; }
		virtual void onSceneChanged(Scene* new_scene, Scene* old_scene) {}
		virtual void onRegisterToNewLookup(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup) {}
	private:
		virtual void onParentChanged(SceneNode* new_parent, SceneNode* old_parent) {}
		SceneNode* getParent() { return _parent; }

		std::string _name;
		Transformation _transformation;
		SceneNode* _parent{ nullptr };
	};
}