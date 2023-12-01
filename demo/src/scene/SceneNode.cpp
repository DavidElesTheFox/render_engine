#include <scene/SceneNode.h>

namespace Scene
{
    SceneNode::SceneNode(std::string name, std::vector<std::unique_ptr<SceneObject>>&& objects)
        : _name(std::move(name))
        , _objects(std::move(objects))
    {
        for (const auto& object : _objects)
        {
            object->changeParent(this);
        }
    }

    void SceneNode::translate(const glm::vec3& value)
    {
        for (auto& obj : _objects)
        {
            obj->getTransformation().translate(value);
        }
    }

    void SceneNode::scale(const glm::vec3& value)
    {
        for (auto& obj : _objects)
        {
            obj->getTransformation().scale(value);
        }
    }

    void SceneNode::rotate(const glm::quat& value)
    {
        for (auto& obj : _objects)
        {
            obj->getTransformation().rotate(value);
        }
    }

    void SceneNode::changeScene(Scene* scene)
    {
        auto* old_scene = _scene;
        _scene = scene;
        for (auto& obj : _objects)
        {
            obj->onSceneChanged(_scene, old_scene);
        }
    }

    void SceneNode::registerToLookUp(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup)
    {
        for (auto& obj : _objects)
        {
            obj->onRegisterToNewLookup(new_lookup, old_lookup);
        }
    }
}