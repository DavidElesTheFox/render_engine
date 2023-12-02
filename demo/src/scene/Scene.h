#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <scene/SceneNode.h>
#include <scene/SceneNodeLookup.h>

#include <glm/vec3.hpp>

namespace Scene
{
    class SceneNode;
    class Camera;
    class Scene
    {
    public:
        struct SceneSetup
        {
            glm::vec3 up;
            glm::vec3 forward;
        };
        Scene(std::string name, SceneSetup scene_setup)
            : _name(std::move(name))
            , _scene_setup(std::move(scene_setup))
        {}

        std::string_view getName() const
        {
            return _name;
        }

        void addNode(std::unique_ptr<SceneNode> node);

        void removeNodeByName(const std::string_view& name)
        {
            std::erase_if(_nodes, [&](const auto& node) { return node->getName() == name; });
        }
        const SceneSetup& getSceneSetup() const { return _scene_setup; }

        void setActiveCamera(Camera* camera) { _active_camera = camera; }
        Camera* getActiveCamera() { return _active_camera; }
        const SceneNodeLookup& getNodeLookup() const { return _node_lookup; }
    private:

        std::string _name;
        std::vector<std::unique_ptr<SceneNode>> _nodes;
        SceneSetup _scene_setup;
        Camera* _active_camera{ nullptr };
        SceneNodeLookup  _node_lookup;
    };



}