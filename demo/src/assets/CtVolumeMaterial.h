#pragma once

#include <memory>

#include <glm/mat4x4.hpp>

#include <render_engine/assets/Image.h>
#include <render_engine/assets/VolumeMaterial.h>

#include <assets/IMaterial.h>
#include <scene/Scene.h>

#include <glm/vec4.hpp>

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

        class FragmentPushConstants
        {
        public:
            void setNumSteps(uint32_t value) { num_steps = value; }
            uint32_t getNumSteps() const { return num_steps; }
            void setStepSize(float value) { step_size = value; }
            float getStepSize() const { return step_size; }
        private:
            uint32_t num_steps{ 100 };
            float step_size{ 0.01f };
        };

        struct MaterialPushConstants
        {
            VertexPushConstants vertex_values;
            FragmentPushConstants fragment_values;
        };

        class Instance : public IInstance
        {
            friend class CtVolumeMaterial;
        public:
            Instance() = default;
            ~Instance() override = default;
            const MaterialPushConstants& getMaterialConstants() const { return _material_constants; }
            MaterialPushConstants& getMaterialConstants() { return _material_constants; }
            RenderEngine::VolumeMaterialInstance* getMaterialInstance() { return _material_instance.get(); }

        private:
            MaterialPushConstants _material_constants;
            std::unique_ptr<RenderEngine::VolumeMaterialInstance> _material_instance;
        };

        struct SegmentationData
        {
            uint8_t threshold{ 0 };
            glm::vec4 color{ 0.0f };
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

        void processImage(RenderEngine::Image* image) const;
        void addSegmentation(SegmentationData segmentation);
    private:
        std::unique_ptr<RenderEngine::VolumeMaterial> _material;
        std::vector<SegmentationData> _segmentations;
    };
}