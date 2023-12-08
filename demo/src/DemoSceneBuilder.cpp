#include "DemoSceneBuilder.h"

#include <ApplicationContext.h>
#include <assets/BillboardMaterial.h>
#include <assets/NoLitMaterial.h>
#include <demo/resource_config.h>
#include <render_engine/RenderContext.h>
#include <render_engine/window/IWindow.h>
#include <scene/Camera.h>
#include <scene/MeshObject.h>
#include <scene/SceneNodeLookup.h>

namespace
{
#pragma region QuadSceneBuilder
    class QuadSceneBuilder
    {
    public:
        QuadSceneBuilder(Assets::AssetDatabase& assets,
                         Scene::Scene& scene,
                         RenderEngine::TextureFactory& texture_factory,
                         RenderEngine::RenderEngine& render_engine)
            : _assets(assets)
            , _scene(scene)
            , _texture_factory(texture_factory)
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
        RenderEngine::TextureFactory& _texture_factory;
        std::unique_ptr<RenderEngine::Texture> _statue_texture;
        RenderEngine::RenderEngine& _render_engine;
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
            camera_builder.add(Scene::Camera::createPerspective(camera_name, glm::radians(fov_degrees), 800.0f, 600.0f, 0.001, 10.0f));

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
                auto& device = RenderEngine::RenderContext::context().getDevice(0);
                auto logical_device = device.getLogicalDevice();
                auto physical_device = device.getPhysicalDevice();
                RenderEngine::SynchronizationPrimitives synchronization_primitives =
                    RenderEngine::SynchronizationPrimitives::CreateWithFence(logical_device);
                RenderEngine::Image image(std::filesystem::path{ IMAGE_BASE } / "statue.jpg");
                auto [texture, transfer_data] = _texture_factory.create(image, VK_IMAGE_ASPECT_COLOR_BIT,
                                                                        VK_SHADER_STAGE_FRAGMENT_BIT,
                                                                        synchronization_primitives,
                                                                        _render_engine.getQueueFamilyIndex(),
                                                                        VK_IMAGE_USAGE_SAMPLED_BIT);
                vkWaitForFences(logical_device, 1, &synchronization_primitives.on_finished_fence, VK_TRUE, UINT64_MAX);
                vkDestroyFence(logical_device, synchronization_primitives.on_finished_fence, nullptr);

                auto billboard_material = _assets.getBaseMaterial<Assets::BillboardMaterial>();
                _statue_texture = std::move(texture);
                auto image_view = _statue_texture->createImageView({});
                RenderEngine::Texture::SamplerData sampler_data{};
                sampler_data.anisotroy_filter_enabled = false;
                sampler_data.border_color = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
                sampler_data.mag_filter = VK_FILTER_LINEAR;
                sampler_data.min_filter = VK_FILTER_LINEAR;
                sampler_data.sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
                sampler_data.unnormalize_coordinate = false;
                auto sampler = _statue_texture->createSampler(sampler_data);

                auto view = std::make_unique<RenderEngine::TextureView>(*_statue_texture, image_view, sampler, physical_device, logical_device);
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
}

DemoSceneBuilder::CreationResult DemoSceneBuilder::buildSceneOfQuads(Assets::AssetDatabase& assets,
                                                                     Scene::Scene& scene,
                                                                     RenderEngine::TextureFactory& texture_factory,
                                                                     RenderEngine::RenderEngine& render_engine)
{
    QuadSceneBuilder scene_builder(assets, scene, texture_factory, render_engine);

    return scene_builder.release();
}
