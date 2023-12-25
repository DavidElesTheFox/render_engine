#pragma once

#include <render_engine/assets/Material.h>

namespace RenderEngine
{
    class MaterialInstance
    {
    public:
        class UpdateContext
        {
        public:
            UpdateContext(PushConstantsUpdater push_constant_updater,
                          const MaterialInstance& material_instance)
                : _push_constant_updater(std::move(push_constant_updater))
                , _material_instance(material_instance)
            {}

            const Shader::MetaData& getVertexShaderMetaData() const
            {
                return _material_instance.getMaterial().getVertexShader().getMetaData();
            }
            const Shader::MetaData& getFragmentShaderMetaData() const
            {
                return _material_instance.getMaterial().getFragmentShader().getMetaData();
            }
            PushConstantsUpdater& getPushConstantUpdater() { return _push_constant_updater; }

        private:
            PushConstantsUpdater _push_constant_updater;
            const MaterialInstance& _material_instance;
        };
        struct CallbackContainer
        {
            std::function<void(UpdateContext& update_context, uint32_t frame_number)> on_frame_begin;
            std::function<void(UpdateContext& update_context, const MeshInstance* mesh_instance)> on_draw;
        };

        MaterialInstance(Material& material,
                         TextureBindingMap texture_bindings,
                         CallbackContainer callbacks,
                         uint32_t id)
            : _material(material)
            , _texture_bindings(std::move(texture_bindings))
            , _callbacks(std::move(callbacks))
            , _id(id)
        {}

        virtual ~MaterialInstance() = default;

        std::unique_ptr<Technique> createTechnique(GpuResourceManager& gpu_resource_manager,
                                                   TextureBindingMap&& subpass_textures,
                                                   VkRenderPass render_pass,
                                                   uint32_t corresponding_subpass) const;

        void onFrameBegin(UpdateContext& update_context, uint32_t frame_number) const
        {
            if (_callbacks.on_frame_begin == nullptr)
            {
                return;
            }
            assert(std::ranges::any_of(_material.getPushConstantsMetaData() | std::views::values,
                                       [](const auto& meta_data)
                                       {
                                           return meta_data.update_frequency == Shader::MetaData::UpdateFrequency::PerFrame
                                               || meta_data.update_frequency == Shader::MetaData::UpdateFrequency::PerDrawCall;
                                       }));
            _callbacks.on_frame_begin(update_context, frame_number);
        }
        void onDraw(UpdateContext& update_context, const MeshInstance* mesh_instance) const
        {
            if (_callbacks.on_draw == nullptr)
            {
                return;
            }
            assert(std::ranges::any_of(_material.getPushConstantsMetaData() | std::views::values,
                                       [](const auto& meta_data) { return meta_data.update_frequency == Shader::MetaData::UpdateFrequency::PerDrawCall; }));

            _callbacks.on_draw(update_context, mesh_instance);
        }
        uint32_t getId() const { return _id; }

        const Material& getMaterial() const { return _material; }
    protected:
        TextureBindingMap cloneBindings() const;
        const CallbackContainer& getCallbackContainer() const { return _callbacks; }
    private:
        Material& _material;
        TextureBindingMap _texture_bindings;
        CallbackContainer _callbacks;
        uint32_t _id{ 0 };
    };
}