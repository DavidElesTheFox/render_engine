#pragma once

#include <memory>

#include <glm/mat4x4.hpp>

#include <render_engine/assets/Image.h>
#include <render_engine/assets/VolumeMaterial.h>

#include <assets/IMaterial.h>
#include <scene/Scene.h>

namespace Assets
{

    class CtVolumeMaterial : public IMaterial
    {
    public:

        class VertexPushConstants
        {
            friend class CtVolumeMaterial;
        public:
        private:
            glm::mat4 projection;
            glm::mat4 model_view;
        };

        struct MaterialPushConstants
        {
            VertexPushConstants vertex_values;
        };
        class Instance : public IInstance
        {
            friend class CtVolumeMaterial;
        public:
            Instance() = default;
            ~Instance() override = default;
            MaterialPushConstants& getMaterialConstants() { return _material_constants; }
            const MaterialPushConstants& getMaterialConstants() const { return _material_constants; }
            RenderEngine::VolumeMaterialInstance* getMaterialInstance() { return _material_instance.get(); }
        private:
            MaterialPushConstants _material_constants;
            std::unique_ptr<RenderEngine::VolumeMaterialInstance> _material_instance;
        };
        static const std::string& GetName()
        {
            static std::string _name = "CtVolume";
            return _name;
        }

        explicit CtVolumeMaterial(uint32_t id);
        ~CtVolumeMaterial() override = default;

        std::unique_ptr<Instance> createInstance(std::unique_ptr<RenderEngine::ITextureView> texture_view, Scene::Scene* scene, uint32_t id);

        RenderEngine::VolumeMaterial* getMaterial() { return _material.get(); }

        const std::string& getName() const override
        {
            return GetName();
        }
    private:
        std::unique_ptr<RenderEngine::VolumeMaterial> _material;
    };
}