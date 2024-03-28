#include "ParallelDemoApplication.h"

#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/UIRenderer.h>

#include <ApplicationContext.h>
#include <DeviceSelector.h>
#include <scene/Camera.h>
#include <WindowSetup.h>

#include <memory>
#include <vector>

namespace
{
    namespace rg = RenderEngine::RenderGraph;

    class ImageAcquireTask : public rg::CpuNode::ICpuTask
    {
    public:
        static std::string imageAvailableSemaphore(uint32_t index)
        {
            return std::vformat("image-available-{:d}", std::make_format_args(index));
        }
        static std::string renderFinishedSemaphore(uint32_t index)
        {
            return std::vformat("image-available-{:d}", std::make_format_args(index));
        }

        ImageAcquireTask(RenderEngine::Window& window, RenderEngine::LogicalDevice& logical_device, uint32_t back_buffer_count)
            : _window(window)
            , _backbuffer_count(back_buffer_count)
        {

        }
        void run(const RenderEngine::SyncObject& in_operations,
                 rg::Job::ExecutionContext& execution_context) final
        {
            if (glfwGetWindowAttrib(_window.getWindowHandle(), GLFW_ICONIFIED) == GLFW_TRUE)
            {
                return;
            }
            if (execution_context.hasRenderTargetIndex())
            {
                return;
            }
            auto& logical_device = _window.getDevice().getLogicalDevice();
            {
                /*
                * This is tricky:
                * There are renders ongoing on the back buffers. Each back buffer has its own semaphore to trigger when the drawing are finished
                * Its gonna be the available_index. Its semaphore will be used to query the next back buffer.
                */
                std::vector<std::string> all_render_finished_semaphore;
                for (uint32_t i = 0; i < _backbuffer_count; ++i)
                {
                    all_render_finished_semaphore.push_back(renderFinishedSemaphore(i));
                }
                uint32_t available_index = in_operations.waitSemaphore(all_render_finished_semaphore, 1);
                auto [call_result, image_index] = in_operations.getOperationsGroup(rg::Link::syncGroup(available_index)).getHostOperations().acquireNextSwapChainImage(logical_device,
                                                                                                                                                                       _window.getSwapChain().getDetails().swap_chain,
                                                                                                                                                                       imageAvailableSemaphore(available_index));
                switch (call_result)
                {
                    case VK_ERROR_OUT_OF_DATE_KHR:
                    case VK_SUBOPTIMAL_KHR:
                        _window.reinitSwapChain();
                        return;
                    case VK_SUCCESS:
                        break;
                    default:
                        throw std::runtime_error("Failed to query swap chain image");
                }
                // If somehow the next image is not the one that is finished we need to wait it until it is not finished.
                if (image_index != available_index)
                {
                    in_operations.waitSemaphore(all_render_finished_semaphore[image_index], 1);
                }
                execution_context.setRenderTargetIndex(image_index);
            }
        }
        bool isActive() const final { return true; }
    private:
        RenderEngine::Window& _window;
        uint32_t _backbuffer_count{ 0 };
    };
}

void ParallelDemoApplication::createRenderEngine()
{
    using RenderEngine::ParallelRenderEngine;
    _render_engine = std::make_unique<ParallelRenderEngine>();

    ParallelRenderEngine::RenderGraphBuilder builder = _render_engine->createRenderGraphBuilder("StandardPipeline");

    ImageAcquireTask* image_acquire_task{ nullptr };
    {
        auto image_acquire_task_ptr = std::make_unique<ImageAcquireTask>();
        image_acquire_task = image_acquire_task_ptr.get();
        builder.addCpuNode("AcquireImage", std::move(image_acquire_task_ptr));
        builder.addRenderNode("ForwardRenderer", std::make_unique<RenderEngine::ForwardRenderer>(*_render_engine, _window_setup->getRenderingWindow().createRenderTarget()));
        builder.addRenderNode("ImguiRenderer", std::make_unique<RenderEngine::UIRenderer>(_window_setup->getUiWindow(), _window_setup->getRenderingWindow().createRenderTarget(), 3));
        builder.addPresentNode("Present", _window_setup->getUiWindow().getSwapChain());
    }
    builder.addCpuSyncLink("AcquireImage", "ForwardRenderer")
        .attachGpuLink("image-available", VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuSyncLink("ForwardRenderer", "ImguiRenderer")
        .attachGpuLink(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuSyncLink("ImguiRenderer", "Present")
        .attachGpuLink(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuAsyncLink("ImguiRenderer", "AcquireImage")
        .attachGpuLink("render-finished", ParallelRenderEngine::RenderGraphBuilder::SemaphoreValue(1));
}

void ParallelDemoApplication::initializeRenderers()
{
    using namespace RenderEngine;

    std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();

    DeviceSelector device_selector;
    RenderContext::InitializationInfo init_info{};
    init_info.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    init_info.app_info.apiVersion = VK_API_VERSION_1_3;
    init_info.app_info.applicationVersion = 0;
    init_info.app_info.engineVersion = 0;
    init_info.app_info.pApplicationName = "DemoApplication";
    init_info.app_info.pEngineName = "FoxEngine";
    init_info.enable_validation_layers = true;
    init_info.enabled_layers = { "VK_LAYER_KHRONOS_validation" };
    init_info.renderer_factory = std::move(renderers);
    init_info.device_selector = [&](const DeviceLookup& lookup) ->VkPhysicalDevice { return device_selector.askForDevice(lookup); };
    init_info.queue_family_selector = [&](const DeviceLookup::DeviceInfo& info) { return device_selector.askForQueueFamilies(info); };
    RenderContext::initialize(std::move(init_info));


}

void ParallelDemoApplication::onGui()
{
    _asset_browser->onGui();
    if (_scene->getActiveCamera())
    {
        _scene->getActiveCamera()->onGui();
    }
    ApplicationContext::instance().onGui();
}

void ParallelDemoApplication::createWindowSetup()
{
    using namespace RenderEngine;
    constexpr bool use_offscreen_rendering = true;

    if constexpr (use_offscreen_rendering)
    {
        _window_setup = std::make_unique<OffScreenWindowSetup>(std::vector<uint32_t> {});
    }
    else
    {
        _window_setup = std::make_unique<SingleWindowSetup>();
    }
    _window_setup->getUiWindow().getRendererAs<UIRenderer>(UIRenderer::kRendererId).setOnGui(
        [&]
        {
            onGui();
        });
}
