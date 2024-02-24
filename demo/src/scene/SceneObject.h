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
        virtual void onSceneChanged(Scene*, Scene*) {}
        virtual void onRegisterToNewLookup(SceneNodeLookup*, SceneNodeLookup*) {}
    private:
        virtual void onParentChanged(SceneNode*, SceneNode*) {}
        SceneNode* getParent() { return _parent; }

        std::string _name;
        Transformation _transformation;
        SceneNode* _parent{ nullptr };
    };
}