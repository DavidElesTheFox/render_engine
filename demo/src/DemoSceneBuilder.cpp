#include "DemoSceneBuilder.h"

#include <ApplicationContext.h>

#include <assets/BillboardMaterial.h>
#include <assets/CtVolumeMaterial.h>
#include <assets/NoLitMaterial.h>

#include <demo/resource_config.h>

#include <render_engine/assets/VolumetricObject.h>
#include <render_engine/RenderContext.h>
#include <render_engine/synchronization/SyncObject.h>
#include <render_engine/window/IWindow.h>

#include <scene/Camera.h>
#include <scene/MeshObject.h>
#include <scene/SceneNodeLookup.h>
#include <scene/VolumeObject.h>

#include <array>
#include <string>
namespace
{


#pragma region QuadSceneBuilder
    class QuadSceneBuilder
    {
    public:
        QuadSceneBuilder(Assets::AssetDatabase& assets,
                         Scene::Scene& scene,
                         RenderEngine::Device& device,
                         RenderEngine::IRenderEngine& render_engine)
            : _assets(assets)
            , _scene(scene)
            , _device(device)
            , _render_engine(render_engine)
        {
            createAssets();
            instantiateMaterials();
            instantiateMeshes();

            loadScene();
        }

        DemoSceneBuilder::CreationResult release()
        {
            auto texture = std::move(_statue_texture);
            _statue_texture = nullptr;
            DemoSceneBuilder::CreationResult result;
            result.textures.push_back(std::move(texture));
            return result;
        }
    private:
        void createGeometries();
        void createBaseMesh();
        void createBaseMaterials();
        void loadScene();
        void instantiateMaterials();
        void instantiateMeshes();

        void createAssets();

        Assets::AssetDatabase& _assets;
        Scene::Scene& _scene;
        RenderEngine::Device& _device;
        std::unique_ptr<RenderEngine::Texture> _statue_texture;
        RenderEngine::IRenderEngine& _render_engine;
    };
    void QuadSceneBuilder::createGeometries()
    {
        {
            auto quad_geometry = std::make_unique<RenderEngine::Geometry>();
            quad_geometry->positions.push_back({ -0.5f, -0.5f, 0.0f });
            quad_geometry->positions.push_back({ 0.5f, -0.5f, 0.0f });
            quad_geometry->positions.push_back({ 0.5f, 0.5f, 0.0f });
            quad_geometry->positions.push_back({ -0.5f, 0.5f, 0.0f });
            quad_geometry->indexes = {
                0, 1, 2, 2, 3, 0
            };
            _assets.addGeometry("quad", std::move(quad_geometry));
        }
        {
            auto quad_geometry = std::make_unique<RenderEngine::Geometry>();
            quad_geometry->positions.push_back({ -0.5f, -0.5f, 0.0f }); quad_geometry->uv.push_back({ 0.0, 0.0f });
            quad_geometry->positions.push_back({ 0.5f, -0.5f, 0.0f }); quad_geometry->uv.push_back({ 1.0, 0.0f });
            quad_geometry->positions.push_back({ 0.5f, 0.5f, 0.0f }); quad_geometry->uv.push_back({ 1.0, 1.0f });
            quad_geometry->positions.push_back({ -0.5f, 0.5f, 0.0f }); quad_geometry->uv.push_back({ 0.0, 1.0f });
            quad_geometry->indexes = {
                0, 1, 2, 2, 3, 0
            };
            _assets.addGeometry("quad_uv", std::move(quad_geometry));
        }
    }

    void QuadSceneBuilder::createBaseMesh()
    {
        {
            auto* nolit_material = _assets.getBaseMaterial<Assets::NoLitMaterial>();
            auto* quad_geometry = _assets.getGeometry("quad");

            auto quad_asset = std::make_unique<RenderEngine::Mesh>(quad_geometry, nolit_material->getMaterial(), ApplicationContext::instance().generateId());
            _assets.addBaseMesh("quad", std::move(quad_asset));
        }
        {
            auto* billboard_material = _assets.getBaseMaterial<Assets::BillboardMaterial>();
            auto* quad_geometry = _assets.getGeometry("quad_uv");

            auto quad_asset = std::make_unique<RenderEngine::Mesh>(quad_geometry, billboard_material->getMaterial(), ApplicationContext::instance().generateId());
            _assets.addBaseMesh("textured_quad", std::move(quad_asset));
        }
    }

    void QuadSceneBuilder::createBaseMaterials()
    {
        _assets.addBaseMaterial(std::make_unique<Assets::NoLitMaterial>(ApplicationContext::instance().generateId()));
        _assets.addBaseMaterial(std::make_unique<Assets::BillboardMaterial>(ApplicationContext::instance().generateId()));
    }


    void QuadSceneBuilder::loadScene()
    {

        {
            Scene::SceneNode::Builder camera_builder;
            const std::string camera_name = "Ortho camera";
            const std::string camera_node_name = "Main camera";
            constexpr float fov_degrees = 60.0f;
            camera_builder.add(Scene::Camera::createPerspective(camera_name,
                                                                glm::radians(fov_degrees), 800.0f, 600.0f, 0.001f, 10.0f));

            _scene.addNode(camera_builder.build(camera_node_name));

            auto* active_camera = _scene.getNodeLookup().findCamera(camera_name);
            _scene.setActiveCamera(active_camera);
            active_camera->getTransformation().setPosition({ 0.0f, 0.0f, 5.0f });
        }
        {
            Scene::SceneNode::Builder mesh_builder;
            auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh00", _assets.getMeshInstance("white_quad00"));
            mesh_object->getTransformation().setPosition(glm::vec3{ 1.0f, -1.0f, 0.0f });
            mesh_builder.add(std::move(mesh_object));
            _scene.addNode(mesh_builder.build("Quad00"));
        }
        {
            Scene::SceneNode::Builder mesh_builder;
            auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh01", _assets.getMeshInstance("white_quad01"));
            mesh_object->getTransformation().setPosition(glm::vec3{ 1.0f, 1.0f, 0.0f });
            mesh_builder.add(std::move(mesh_object));
            _scene.addNode(mesh_builder.build("Quad01"));
        }
        {
            Scene::SceneNode::Builder mesh_builder;
            auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh02", _assets.getMeshInstance("red_quad00"));
            mesh_object->getTransformation().setPosition(glm::vec3{ -1.0f, 1.0f, 0.0f });
            mesh_builder.add(std::move(mesh_object));
            _scene.addNode(mesh_builder.build("Quad02"));
        }
        {
            Scene::SceneNode::Builder mesh_builder;
            auto mesh_object = std::make_unique<Scene::MeshObject>("QuadMesh03", _assets.getMeshInstance("red_quad01"));
            mesh_object->getTransformation().setPosition(glm::vec3{ -1.0f, -1.0f, 0.0f });
            mesh_builder.add(std::move(mesh_object));
            _scene.addNode(mesh_builder.build("Quad03"));
        }
        {
            Scene::SceneNode::Builder mesh_builder;
            auto mesh_object = std::make_unique<Scene::MeshObject>("StatueQuadMesh", _assets.getMeshInstance("statue_quad01"));
            mesh_object->getTransformation().setPosition(glm::vec3{ -3.0f, 1.0f, 0.0f });
            mesh_builder.add(std::move(mesh_object));
            _scene.addNode(mesh_builder.build("StatueQuad"));
        }
    }

    void QuadSceneBuilder::createAssets()
    {
        createGeometries();
        createBaseMaterials();
        createBaseMesh();
    }

    void QuadSceneBuilder::instantiateMaterials()
    {
        {
            auto nolit_material = _assets.getBaseMaterial<Assets::NoLitMaterial>();
            _assets.addMaterialInstance("NoLit - white", nolit_material->createInstance(glm::vec3{ 1.0f }, &_scene, ApplicationContext::instance().generateId()));
            _assets.addMaterialInstance("NoLit - red", nolit_material->createInstance(glm::vec3{ 1.0f, 0.0f, 0.0f }, &_scene, ApplicationContext::instance().generateId()));
        }
        {
            {
                auto& logical_device = _device.getLogicalDevice();
                RenderEngine::SyncObject sync_object(logical_device, "QuadSceneUpload");
                RenderEngine::Image image(std::filesystem::path{ IMAGE_BASE } / "statue.jpg");
                auto texture = _device.getTextureFactory().create(image, VK_IMAGE_ASPECT_COLOR_BIT,
                                                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                  sync_object.getOperationsGroup(RenderEngine::SyncGroups::kInternal),
                                                                  &_render_engine.getTransferCommandContext(),
                                                                  VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                  RenderEngine::TextureState{}.setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                                                  .setPipelineStage(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT)
                                                                  .setAccessFlag(VK_ACCESS_2_SHADER_READ_BIT));
                // TODO remove this and do proper synchronization with render graph
                _device.getDataTransferContext().synchronizeScheduler({});
                texture->getUploadTask()->wait();

                auto billboard_material = _assets.getBaseMaterial<Assets::BillboardMaterial>();
                _statue_texture = std::move(texture);
                RenderEngine::Texture::SamplerData sampler_data{};

                auto view = _statue_texture->createTextureView({}, sampler_data);
                _assets.addMaterialInstance("Billboard - statue", billboard_material->createInstance(std::move(view), &_scene, ApplicationContext::instance().generateId()));
            }
        }
    }

    void QuadSceneBuilder::instantiateMeshes()
    {

        auto quad_mesh = _assets.getBaseMesh("quad");
        auto textured_quad_mesh = _assets.getBaseMesh("textured_quad");
        {
            auto quad = std::make_unique<RenderEngine::MeshInstance>(quad_mesh,
                                                                     _assets.getMaterialInstanceAs<Assets::NoLitMaterial::Instance>("NoLit - white")->getMaterialInstance(),
                                                                     ApplicationContext::instance().generateId());
            _assets.addMeshInstance("white_quad00", std::move(quad));
        }
        {
            auto quad = std::make_unique<RenderEngine::MeshInstance>(quad_mesh,
                                                                     _assets.getMaterialInstanceAs<Assets::NoLitMaterial::Instance>("NoLit - white")->getMaterialInstance(),
                                                                     ApplicationContext::instance().generateId());
            _assets.addMeshInstance("white_quad01", std::move(quad));
        }
        {
            auto quad = std::make_unique<RenderEngine::MeshInstance>(quad_mesh,
                                                                     _assets.getMaterialInstanceAs<Assets::NoLitMaterial::Instance>("NoLit - red")->getMaterialInstance(),
                                                                     ApplicationContext::instance().generateId());
            _assets.addMeshInstance("red_quad00", std::move(quad));
        }
        {
            auto quad = std::make_unique<RenderEngine::MeshInstance>(quad_mesh,
                                                                     _assets.getMaterialInstanceAs<Assets::NoLitMaterial::Instance>("NoLit - red")->getMaterialInstance(),
                                                                     ApplicationContext::instance().generateId());
            _assets.addMeshInstance("red_quad01", std::move(quad));
        }

        {
            auto textured_quad = std::make_unique<RenderEngine::MeshInstance>(textured_quad_mesh,
                                                                              _assets.getMaterialInstanceAs<Assets::BillboardMaterial::Instance>("Billboard - statue")->getMaterialInstance(),
                                                                              ApplicationContext::instance().generateId());
            _assets.addMeshInstance("statue_quad01", std::move(textured_quad));
        }
    }
#pragma endregion

#pragma region VolumetricSceneBuilder

    struct ImageData
    {
        std::filesystem::path folder_name;
        uint32_t image_count;
    };

    class VolumetricSceneBuilder
    {
    public:
        VolumetricSceneBuilder(Assets::AssetDatabase& assets,
                               Scene::Scene& scene,
                               RenderEngine::Device& device,
                               RenderEngine::IRenderEngine& render_engine,
                               bool use_ao)
            : _assets(assets)
            , _scene(scene)
            , _device(device)
            , _render_engine(render_engine)
            , _use_ao(use_ao)
        {
            createAssets();
            instantiateMaterials();
            instantiateMeshes();

            loadScene();
        }

        DemoSceneBuilder::CreationResult release()
        {
            auto texture = std::move(_ct_texture);
            _ct_texture = nullptr;
            DemoSceneBuilder::CreationResult result;
            result.textures.push_back(std::move(texture));
            return result;
        }
    private:
        void createBaseMesh();
        void createBaseMaterials();
        void loadScene();
        void instantiateMaterials();
        void instantiateMeshes();

        void createAssets();

        Assets::AssetDatabase& _assets;
        Scene::Scene& _scene;
        RenderEngine::Device& _device;
        std::unique_ptr<RenderEngine::Texture> _ct_texture;
        RenderEngine::IRenderEngine& _render_engine;
        bool _use_ao{ false };
    };

    void VolumetricSceneBuilder::createBaseMesh()
    {
        {
            auto* ct_material = _assets.getBaseMaterial<Assets::CtVolumeMaterial>();

            auto ct_asset = RenderEngine::VolumetricObject::create(glm::vec3{ 1.0, 1.0, 1.0 }, ct_material->getMaterial(), ApplicationContext::instance().generateId());
            _assets.addBaseMesh("ct_asset", std::move(ct_asset));
        }
    }

    void VolumetricSceneBuilder::createBaseMaterials()
    {
        const glm::vec4 bone{ 0.89f, 0.85f, 0.78f, 1.f };
        auto ct_material = std::make_unique<Assets::CtVolumeMaterial>(_use_ao, ApplicationContext::instance().generateId());
        ct_material->addSegmentation({ .threshold = 200, .color = bone });
        _assets.addBaseMaterial(std::move(ct_material));
    }

    void VolumetricSceneBuilder::loadScene()
    {
        {
            Scene::SceneNode::Builder camera_builder;
            const std::string camera_name = "Perspective camera";
            const std::string camera_node_name = "Main camera";
            constexpr float fov_degrees = 60.0f;
            camera_builder.add(Scene::Camera::createPerspective(camera_name, glm::radians(fov_degrees), 800.0f, 600.0f, 0.001f, 10.0f));

            _scene.addNode(camera_builder.build(camera_node_name));

            auto* active_camera = _scene.getNodeLookup().findCamera(camera_name);
            _scene.setActiveCamera(active_camera);
            active_camera->getTransformation().setPosition({ 0.0f, 0.0f, 5.0f });
        }
        {
            Scene::SceneNode::Builder mesh_builder;
            auto mesh_object = std::make_unique<Scene::VolumeObject>("CtFingerMesh", static_cast<RenderEngine::VolumetricObjectInstance*>(_assets.getMeshInstance("ct_finger")));
            mesh_object->getTransformation().setPosition(glm::vec3{ 0.0f, 0.0f, 2.0f });
            mesh_object->getTransformation().setEulerAngles(glm::radians(glm::vec3{ -60.0f, 10.0, 2.5f }));
            mesh_builder.add(std::move(mesh_object));
            _scene.addNode(mesh_builder.build("CtFinger"));
        }
    }

    void VolumetricSceneBuilder::createAssets()
    {
        createBaseMaterials();
        createBaseMesh();
    }

    void VolumetricSceneBuilder::instantiateMaterials()
    {


        std::array<ImageData, 2> ct_image_container =
        {
            ImageData{.folder_name = "ct_finger_small", .image_count = 86},
            ImageData{.folder_name = "iliac_acetabulum_small", .image_count = 131}
        };

        const auto& image_data = ct_image_container[1];

        auto& logical_device = _device.getLogicalDevice();
        auto ct_material = _assets.getBaseMaterial<Assets::CtVolumeMaterial>();

        std::filesystem::path ct_base_path{ IMAGE_BASE };
        ct_base_path /= image_data.folder_name;

        std::vector<std::filesystem::path> ct_image_path_container;
        for (uint32_t i = 0; i < image_data.image_count; ++i)
        {
            ct_image_path_container.push_back(ct_base_path / std::format("IMG-0003-{:0>5d}.jpg", i + 1));
        }

        RenderEngine::SyncObject sync_object(logical_device, "VolumetricSceneUpload");
        RenderEngine::Image image_3d(ct_image_path_container);

        if (_use_ao)
        {
            auto texture = _device.getTextureFactory().createExternal(image_3d, VK_IMAGE_ASPECT_COLOR_BIT,
                                                                      VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                      sync_object.getOperationsGroup(RenderEngine::SyncGroups::kInternal),
                                                                      &_render_engine.getTransferCommandContext(),
                                                                      VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                      RenderEngine::TextureState{}.setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                                                      .setPipelineStage(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT)
                                                                      .setAccessFlag(VK_ACCESS_2_SHADER_READ_BIT));
            // TODO remove this and do proper synchronization with render graph
            _device.getDataTransferContext().synchronizeScheduler({});
            texture->getUploadTask()->wait();
            _ct_texture = std::move(texture);

        }
        else
        {
            ct_material->processImage(&image_3d);

            auto texture = _device.getTextureFactory().create(image_3d, VK_IMAGE_ASPECT_COLOR_BIT,
                                                              VK_SHADER_STAGE_FRAGMENT_BIT,
                                                              sync_object.getOperationsGroup(RenderEngine::SyncGroups::kInternal),
                                                              &_render_engine.getTransferCommandContext(),
                                                              VK_IMAGE_USAGE_SAMPLED_BIT,
                                                              RenderEngine::TextureState{}.setImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                                              .setPipelineStage(VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT)
                                                              .setAccessFlag(VK_ACCESS_2_SHADER_READ_BIT));
            // TODO remove this and do proper synchronization with render graph
            _device.getDataTransferContext().synchronizeScheduler({});
            texture->getUploadTask()->wait();
            _ct_texture = std::move(texture);


        }



        RenderEngine::Texture::SamplerData sampler_data{};
        sampler_data.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        auto view = _ct_texture->createTextureView({}, sampler_data);
        _assets.addMaterialInstance("CtVolume - ct_finger",
                                    ct_material->createInstance(std::move(view), &_scene, ApplicationContext::instance().generateId()));
    }

    void VolumetricSceneBuilder::instantiateMeshes()
    {

        auto ct_asset = _assets.getBaseMesh("ct_asset");
        {
            auto ct_finger = std::make_unique<RenderEngine::MeshInstance>(ct_asset,
                                                                          _assets.getMaterialInstanceAs<Assets::CtVolumeMaterial::Instance>("CtVolume - ct_finger")->getMaterialInstance(),
                                                                          ApplicationContext::instance().generateId());
            _assets.addMeshInstance("ct_finger", std::move(ct_finger));
        }

    }

#pragma endregion
}

DemoSceneBuilder::CreationResult DemoSceneBuilder::buildSceneOfQuads(Assets::AssetDatabase& assets,
                                                                     Scene::Scene& scene,
                                                                     RenderEngine::Device& device,
                                                                     RenderEngine::IRenderEngine& render_engine)
{
    QuadSceneBuilder scene_builder(assets, scene, device, render_engine);

    return scene_builder.release();
}

DemoSceneBuilder::CreationResult DemoSceneBuilder::buildVolumetricScene(Assets::AssetDatabase& assets,
                                                                        Scene::Scene& scene,
                                                                        RenderEngine::Device& device,
                                                                        RenderEngine::IRenderEngine& render_engine,
                                                                        bool use_ao)
{
    VolumetricSceneBuilder scene_builder(assets, scene, device, render_engine, use_ao);

    return scene_builder.release();
}
