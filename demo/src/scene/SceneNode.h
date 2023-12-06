#pragma once

#include <algorithm>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include <scene/Scene.h>
#include <scene/SceneObject.h>

namespace Scene
{
    class SceneNode
    {
    public:
        class Builder
        {
        public:
            Builder()
            {}

            void add(std::unique_ptr<SceneObject>&& object)
            {
                _objects.emplace_back(std::move(object));
            }

            void removeByName(const std::string_view& name)
            {
                std::erase_if(_objects, [&](const auto& obj) { return obj->getName() == name; });
            }

            std::unique_ptr<SceneNode> build(std::string name)
            {
                return std::make_unique<SceneNode>(std::move(name), std::move(_objects));
            }
        private:
            std::vector<std::unique_ptr<SceneObject>> _objects;
        };

        class SceneAccessor
        {
            friend class Scene;
        public:
            explicit SceneAccessor(SceneNode* scene_node)
                : _scene_node(scene_node)
            {}
        private:
            Scene* getScene()
            {
                return _scene_node->_scene;
            }

            SceneNode* _scene_node;
        };
        SceneNode(std::string name, std::vector<std::unique_ptr<SceneObject>>&& objects);

        std::ranges::random_access_range auto&& getObjects() const
        {
            return _objects | std::views::transform([](const auto& ptr) { return ptr.get(); });
        }
        std::ranges::random_access_range auto&& getObjects()
        {
            return _objects | std::views::transform([](auto& ptr) { return ptr.get(); });
        }

        std::string_view getName() const
        {
            return _name;
        }
        void translate(const glm::vec3& value);
        void scale(const glm::vec3& value);
        void rotate(const glm::quat& value);

        void changeScene(Scene* scene);
        void registerToLookUp(SceneNodeLookup* new_lookup, SceneNodeLookup* old_lookup);

        const Scene* getScene() const { return _scene; }
        SceneAccessor accessToScene() { return SceneAccessor{ this }; }
    private:
        std::string _name;
        std::vector<std::unique_ptr<SceneObject>> _objects;
        Scene* _scene{ nullptr };
    };
}