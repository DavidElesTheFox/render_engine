#pragma once
#include <memory>

#include <glm/mat4x4.hpp>

#include <render_engine/assets/Material.h>
#include <render_engine/resources/Texture.h>

#include <assets/IMaterial.h>
#include <scene/Scene.h>

namespace Assets
{

    class BillboardMaterial : public IMaterial
    {
    public:

        class VertexPushConstants
        {
            friend class BillboardMaterial;
        public:
        private:
            glm::mat4 projection;
            glm::mat4 model_view;
        };


        struct MaterialPushConstants
        {
            VertexPushConstants vertex_values{};
        };
        class Instance : public IInstance
        {
            friend class BillboardMaterial;
        public:
            Instance() = default;
            ~Instance() override = default;
            MaterialPushConstants& getMaterialConstants() { return _material_constants; }
            const MaterialPushConstants& getMaterialConstants() const { return _material_constants; }
            RenderEngine::MaterialInstance* getMaterialInstance() { return _material_instance.get(); }
        private:
            MaterialPushConstants _material_constants{};
            std::unique_ptr<RenderEngine::MaterialInstance> _material_instance;
        };
        static const std::string& GetName()
        {
            static std::string _name = "Billboard";
            return _name;
        }

        explicit BillboardMaterial(uint32_t id);
        ~BillboardMaterial() override = default;

        std::unique_ptr<Instance> createInstance(std::unique_ptr<RenderEngine::TextureView> texture, Scene::Scene* scene, uint32_t id);

        RenderEngine::Material* getMaterial() { return _material.get(); }

        const std::string& getName() const override
        {
            return GetName();
        }
    private:
        std::unique_ptr<RenderEngine::Material> _material;
    };
}