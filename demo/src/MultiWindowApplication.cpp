#include "MultiWindowApplication.h"

#include <render_engine/renderers/ImageStreamRenderer.h>
#include <render_engine/window/Window.h>

#include<demo/resource_config.h>

#include<DeviceSelector.h>

#include <format>

void MultiWindowApplication::init()
{
    initEngine();
    initImages();
#if ENABLE_WINDOW_0
    _window_0 = RenderEngine::RenderContext::context().getDevice(kPrimaryDeviceIndex).createWindow(std::format("Example Renderer (on device {})", kPrimaryDeviceIndex), 3);
    _window_0->registerRenderers({ RenderEngine::ExampleRenderer::kRendererId });
#endif
#if ENABLE_WINDOW_1

    _window_1 = RenderEngine::RenderContext::context().getDevice(kSecondaryDeviceIndex).createWindow(std::format("Example Renderer with UI (on device {})", kSecondaryDeviceIndex), 3);
    _window_1->registerRenderers({ RenderEngine::ExampleRenderer::kRendererId, RenderEngine::UIRenderer::kRendererId });
#endif
#if ENABLE_WINDOW_2
    _window_2 = RenderEngine::RenderContext::context().getDevice(kPrimaryDeviceIndex).createWindow(std::format("UI only (on device {})", kPrimaryDeviceIndex), 3);
    _window_2->registerRenderers({ RenderEngine::UIRenderer::kRendererId });
#endif
#if ENABLE_WINDOW_3
    _window_3 = RenderEngine::RenderContext::context().getDevice(kSecondaryDeviceIndex).createWindow(std::format("Image Stream Renderer (on device {})", kSecondaryDeviceIndex), 3);
    _window_3->registerRenderers({ RenderEngine::ImageStreamRenderer::kRendererId });
#endif

#if ENABLE_WINDOW_0
    auto& example_renderer_0 = _window_0->getRendererAs<RenderEngine::ExampleRenderer>(RenderEngine::ExampleRenderer::kRendererId);
#endif
#if ENABLE_WINDOW_1

    auto& ui_renderer_1 = _window_1->getRendererAs<RenderEngine::UIRenderer>(RenderEngine::UIRenderer::kRendererId);
    auto& example_renderer_1 = _window_1->getRendererAs<RenderEngine::ExampleRenderer>(RenderEngine::ExampleRenderer::kRendererId);
#endif
#if ENABLE_WINDOW_2
    auto& ui_renderer_2 = _window_2->getRendererAs<RenderEngine::UIRenderer>(RenderEngine::UIRenderer::kRendererId);
#endif
    {
        const std::vector<RenderEngine::ExampleRenderer::Vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
        };
        const std::vector<uint16_t> indices = {
            0, 1, 2, 2, 3, 0
        };
#if ENABLE_WINDOW_0

        example_renderer_0.init(vertices, indices);
#endif
#if ENABLE_WINDOW_1

        example_renderer_1.init(vertices, indices);
        example_renderer_1.getColorOffset().r = 1.f;
        ui_renderer_1.setOnGui([&]
                               {
                                   float value = example_renderer_1.getColorOffset().r;
                                   ImGui::SliderFloat("Color offset", &value, 0.0f, 1.0f);
                                   example_renderer_1.getColorOffset().r = value;

                               });
#endif
#if ENABLE_WINDOW_2
        ui_renderer_2.setOnGui([] { ImGui::ShowDemoWindow(); });
#endif
    }
}

void MultiWindowApplication::run()
{

    bool has_open_window = true;
    while (has_open_window)
    {
        updateImageStream();

        has_open_window = false;
#if ENABLE_WINDOW_0
        _window_0->update();
        has_open_window |= _window_0->isClosed() == false;
#endif
#if ENABLE_WINDOW_1
        _window_1->update();
        has_open_window |= _window_1->isClosed() == false;
#endif
#if ENABLE_WINDOW_2
        _window_2->update();
        has_open_window |= _window_2->isClosed() == false;
#endif
#if ENABLE_WINDOW_3
        _window_3->update();
        has_open_window |= _window_3->isClosed() == false;
#endif
    }
}

void MultiWindowApplication::initEngine()
{
    using namespace RenderEngine;
    std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();
    renderers->registerRenderer(UIRenderer::kRendererId,
                                [](auto& window, auto render_target, uint32_t back_buffer_count, AbstractRenderer* previous_renderer, bool) -> std::unique_ptr<AbstractRenderer>
                                {
                                    const bool first_renderer = previous_renderer == nullptr;
                                    if (first_renderer)
                                    {
                                        render_target = render_target.clone()
                                            .changeLoadOperation(VK_ATTACHMENT_LOAD_OP_CLEAR)
                                            .changeInitialLayout(VK_IMAGE_LAYOUT_UNDEFINED);
                                    }
                                    else
                                    {
                                        render_target = render_target.clone()
                                            .changeLoadOperation(VK_ATTACHMENT_LOAD_OP_LOAD)
                                            .changeInitialLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                                    }
                                    return std::make_unique<UIRenderer>(static_cast<Window&>(window), render_target, back_buffer_count);
                                });

    renderers->registerRenderer(ExampleRenderer::kRendererId,
                                [](auto& window, auto render_target, uint32_t back_buffer_count, AbstractRenderer*, bool last_renderer) -> std::unique_ptr<AbstractRenderer>
                                {
                                    if (last_renderer == false)
                                    {
                                        render_target = render_target.clone().changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                                    }
                                    return std::make_unique<ExampleRenderer>(window.getRenderEngine(), render_target, back_buffer_count);
                                });
    renderers->registerRenderer(ImageStreamRenderer::kRendererId,
                                [&](auto& window, auto render_target, uint32_t back_buffer_count, AbstractRenderer*, bool last_renderer) -> std::unique_ptr<AbstractRenderer>
                                {
                                    if (last_renderer == false)
                                    {
                                        render_target = render_target.clone().changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                                    }
                                    return std::make_unique<ImageStreamRenderer>(window.getRenderEngine(), *_image_stream, render_target, back_buffer_count);
                                });
    constexpr bool enable_multiple_device_selection = kPrimaryDeviceIndex != kSecondaryDeviceIndex;
    DeviceSelector device_selector(enable_multiple_device_selection);

    RenderContext::InitializationInfo init_info{};
    init_info.app_info.apiVersion = VK_API_VERSION_1_3;
    init_info.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    init_info.app_info.applicationVersion = 0;
    init_info.app_info.engineVersion = 0;
    init_info.app_info.pApplicationName = "MultiWindow Application";
    init_info.app_info.pEngineName = "FoxEngine";
    init_info.enable_validation_layers = true;
    init_info.enabled_layers = { "VK_LAYER_KHRONOS_validation" };
    init_info.renderer_factory = std::move(renderers);
    init_info.device_selector = [&](const DeviceLookup& lookup) ->VkPhysicalDevice { return device_selector.askForDevice(lookup); };
    init_info.queue_family_selector = [&](const DeviceLookup::DeviceInfo& info) { return device_selector.askForQueueFamilies(info); };
    RenderContext::initialize(std::move(init_info));
}

void MultiWindowApplication::initImages()
{
    const size_t last_image_idx = 59;
    const std::filesystem::path image_base_dir{ IMAGE_BASE };
    const auto base_dir = image_base_dir / "pink_panther";
    for (size_t i = 0; i <= last_image_idx; ++i)
    {
        _images.push_back(RenderEngine::Image(base_dir / std::format("{:0>3}.png", i)));
    }
    RenderEngine::ImageStream::ImageDescription image_description{
        .width = _images[0].getWidth(),
        .height = _images[0].getHeight(),
        .format = _images[0].getFormat()
    };
    _image_stream = std::make_unique<RenderEngine::ImageStream>(image_description);
}

void MultiWindowApplication::updateImageStream()
{
    using namespace std::chrono_literals;
    const auto now = std::chrono::steady_clock::now();
    static const auto frame_duration = 1000ms / 30;
    if (now > _last_image_update + frame_duration)
    {
        // TODO support Depth/Stencil buffer data
        // We assume the image has uint8_t data
        (*_image_stream) << std::get<std::vector<uint8_t>>(_images[_current_image].getData());
        _last_image_update = now;
        _current_image = (_current_image + 1) % _images.size();
    }
}

