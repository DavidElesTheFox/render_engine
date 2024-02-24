#include "OffScreenTestApplication.h"

#include <render_engine/Device.h>
#include <render_engine/RenderContext.h>
#include <render_engine/renderers/ForwardRenderer.h>

#include <DeviceSelector.h>
namespace
{
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
}
void OffScreenTestApplication::init()
{
    using namespace RenderEngine;

    initializeRenderers();
    createWindow();

    createScene();

    DemoSceneBuilder demoSceneBuilder;
    _scene_resources = demoSceneBuilder.buildSceneOfQuads(_assets,
                                                          *_scene,
                                                          _window->getDevice(),
                                                          _window->getRenderEngine());

    _render_manager = std::make_unique<Scene::SceneRenderManager>(_scene->getNodeLookup(), *_window);

    _render_manager->registerMeshesForRender();

    _asset_browser = std::make_unique<Ui::AssetBrowserUi>(_assets, *_scene);
}

void OffScreenTestApplication::run()
{
    auto image_description = _window->getImageStream().getImageDescription();
    while (_window->isClosed() == false)
    {
        _window->update();
        std::vector<uint8_t> data;
        _window->getImageStream() >> data;
        if (data.empty() == false && _save_output)
        {
            stbi_write_jpg(std::format("{:d}.jpg", _window->getFrameCounter() % 10).c_str(),
                           image_description.width,
                           image_description.height,
                           4,
                           data.data(), 100);
        }
    }
}

OffScreenTestApplication::~OffScreenTestApplication()
{
    if (_window != nullptr)
    {
        // finish all rendering before destroying anything
        _window->getDevice().waitIdle();
    }
}

void OffScreenTestApplication::createScene()
{
    Scene::Scene::SceneSetup scene_setup{
        .up = glm::vec3(0.0f, 1.0f, 0.0f),
        .forward = glm::vec3(0.0f, 0.0f, -1.0f)
    };

    _scene = std::make_unique<Scene::Scene>("Simple Scene", std::move(scene_setup));
}

void OffScreenTestApplication::initializeRenderers()
{
    using namespace RenderEngine;

    std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();

    renderers->registerRenderer(ForwardRenderer::kRendererId,
                                [&](auto& window, auto render_target, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool has_next) -> std::unique_ptr<AbstractRenderer>
                                {
                                    return std::make_unique<ForwardRenderer>(window, render_target, has_next);
                                });

    DeviceSelector device_selector;
    RenderContext::InitializationInfo init_info{};
    init_info.app_info.apiVersion = VK_API_VERSION_1_3;
    init_info.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    init_info.app_info.applicationVersion = 0;
    init_info.app_info.engineVersion = 0;
    init_info.app_info.pApplicationName = "OffScreenTest";
    init_info.app_info.pEngineName = "FoxEngine";
    init_info.enable_validation_layers = true;
    init_info.enabled_layers = { "VK_LAYER_KHRONOS_validation" };
    init_info.renderer_factory = std::move(renderers);
    init_info.device_selector = [&](const DeviceLookup& lookup) ->VkPhysicalDevice { return device_selector.askForDevice(lookup); };
    init_info.queue_family_selector = [&](const DeviceLookup::DeviceInfo& info) { return device_selector.askForQueueFamilies(info); };
    RenderContext::initialize(std::move(init_info));
}


void OffScreenTestApplication::createWindow()
{
    using namespace RenderEngine;

    _window = RenderContext::context().getDevice(0).createOffScreenWindow("Offscreen window", 3);
    _window->registerRenderers({ ForwardRenderer::kRendererId });

}

