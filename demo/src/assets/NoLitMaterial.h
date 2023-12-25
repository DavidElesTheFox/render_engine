#pragma once
#include <memory>

#include <glm/mat4x4.hpp>

#include <render_engine/assets/Material.h>
#include <render_engine/assets/MaterialInstance.h>

#include <assets/IMaterial.h>
#include <scene/Scene.h>

namespace Assets
{

    class NoLitMaterial : public IMaterial
    {
    public:

        class VertexPushConstants
        {
            friend class NoLitMaterial;
        public:
        private:
            glm::mat4 projection;
            glm::mat4 model_view;
        };
        class FragmentPushConstants
        {
            friend class NoLitMaterial;
        public:
            const glm::vec3& getInstanceColor() const { return instance_color; }
            void setInstanceColor(glm::vec3 color) { instance_color = std::move(color); }
        private:
            glm::vec3 instance_color{ 1.0f };
        };

        struct MaterialPushConstants
        {
            VertexPushConstants vertex_values;
            FragmentPushConstants fragment_values;
        };
        class Instance : public IInstance
        {
            friend class NoLitMaterial;
        public:
            Instance() = default;
            ~Instance() override = default;
            MaterialPushConstants& getMaterialConstants() { return _material_constants; }
            const MaterialPushConstants& getMaterialConstants() const { return _material_constants; }
            RenderEngine::MaterialInstance* getMaterialInstance() { return _material_instance.get(); }
        private:
            MaterialPushConstants _material_constants;
            std::unique_ptr<RenderEngine::MaterialInstance> _material_instance;
        };
        static const std::string& GetName()
        {
            static std::string _name = "NoLit";
            return _name;
        }

        explicit NoLitMaterial(uint32_t id);
        ~NoLitMaterial() override = default;

        std::unique_ptr<Instance> createInstance(glm::vec3 instance_color, Scene::Scene* scene, uint32_t id);

        RenderEngine::Material* getMaterial() { return _material.get(); }

        const std::string& getName() const override
        {
            return GetName();
        }
    private:
        std::unique_ptr<RenderEngine::Material> _material;
    };
}