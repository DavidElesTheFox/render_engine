#include <scene/Scene.h>

namespace Scene
{
	void Scene::addNode(std::unique_ptr<SceneNode> node)
	{
		auto* old_scene = node->accessToScene().getScene();

		_nodes.emplace_back(std::move(node));
		_nodes.back()->registerToLookUp(&_node_lookup,
			old_scene == nullptr ? nullptr : &old_scene->_node_lookup);
		_nodes.back()->changeScene(this);
	}
}