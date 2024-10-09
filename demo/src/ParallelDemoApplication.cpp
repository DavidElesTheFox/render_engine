#include "ParallelDemoApplication.h"

#include <render_engine/containers/IndexSet.h>
#include <render_engine/debug/Profiler.h>
#include <render_engine/renderers/ForwardRenderer.h>
#include <render_engine/renderers/UIRenderer.h>
#include <render_engine/rendergraph/cputasks/ImageAquireTask.h>
#include <render_engine/rendergraph/Node.h>
#include <render_engine/rendergraph/Topic.h>
#include <render_engine/synchronization/SyncObject.h>

#include <ApplicationContext.h>
#include <DeviceSelector.h>
#include <scene/Camera.h>
#include <WindowSetup.h>

#include <format>
#include <memory>
#include <thread>
#include <vector>
namespace
{
#include <stb_image_write.h>
}
namespace RenderEngine
{



    RenderTargetTextures::RenderTargetTextures(std::vector<std::unique_ptr<Texture>>&& textures)
        : _textures(std::move(textures))
    {
        Texture::ImageViewData image_view_data;

        for (uint32_t i = 0; i < _textures.size(); ++i)
        {
            auto& texture = *_textures[i];
            _texture_views.push_back(texture.createTextureView(image_view_data, std::nullopt));
        }
    }
    RenderTarget RenderTargetTextures::createRenderTarget() const
    {
        using namespace views;
        auto image_views = _texture_views | to_raw_pointer | to<std::vector<ITextureView*>>();
        auto& image_prototype = _textures.front()->getImage();
        return { std::move(image_views),
            image_prototype.getWidth(),
            image_prototype.getHeight(),
            image_prototype.getFormat(),
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE
        };
    }
    OffscreenSwapChain::OffscreenSwapChain(RefObj<Device> device, RefObj<RenderTargetTextures> render_target_textures)
        : _device(std::move(device))
        , _render_target_textures(std::move(render_target_textures))
        , _available_render_target_indexes(std::views::iota(0u, _render_target_textures->getSize()) | views::to<std::deque<uint32_t>>())
    {

    }

    uint32_t OffscreenSwapChain::acquireImageIndex(std::chrono::milliseconds timeout)
    {
        std::unique_lock lock(_available_render_target_indexes_mutex);
        if (_available_render_target_indexes.empty())
        {
            bool ok = _render_target_available.wait_for(lock, timeout, [&] { return _available_render_target_indexes.empty() == false; });
            if (ok == false)
            {
                throw std::runtime_error("Couldn't wait for next image");
            }
        }
        uint32_t result = _available_render_target_indexes.front();
        _available_render_target_indexes.pop_front();
        return result;
    }
    void OffscreenSwapChain::present(uint32_t render_target_index, const SyncOperations& sync_operations)
    {
        _device->getDataTransferContext().getScheduler().download(&_render_target_textures->getTexture(render_target_index),
                                                                  sync_operations);
    }
    void OffscreenSwapChain::readback(uint32_t render_target_index, ImageStream& stream)
    {
        auto download_task = _render_target_textures->getTexture(render_target_index).clearDownloadTask();
        auto image = download_task->getImage();
        stream << std::move(std::get<std::vector<uint8_t>>(image.getData()));
        {
            std::unique_lock lock(_available_render_target_indexes_mutex);
            _available_render_target_indexes.push_back(render_target_index);
        }
        _render_target_available.notify_one();
    }
}

namespace RenderEngine::RenderGraph
{
    namespace
    {
        class OfflineImageAcquireTask : public CpuNode::ICpuTask
        {
        public:
            explicit OfflineImageAcquireTask(RefObj<OffscreenSwapChain> swap_chain)
                : _swap_chain(std::move(swap_chain))
            {}
            void run(CpuNode& self, ExecutionContext& execution_context) final
            {
                using namespace std::chrono_literals;
                const auto timeout = 1s;
                auto current_index = execution_context.getPoolIndex();
                current_index.render_target_index = _swap_chain->acquireImageIndex(timeout);
                execution_context.setPoolIndex(current_index);

                auto& texture = _swap_chain->getRenderTargetTextures().getTexture(current_index.render_target_index);
                ResourceStateMachine state_machine(_swap_chain->getDevice().getLogicalDevice());
                state_machine.recordStateChange(&texture,
                                                texture.getResourceState().clone().setImageLayout(VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL));
                /*
                    TODO: WAT overview of the problem
                    Remove BaseGpuNode.The problem is not about the resource state or transition.The state gonna be set by the first render_pass who touches the texture.
                    This logic is removed recently from the renderer.but Should be added back in a more generic way.
                    Create the textures with an empty upload ? Or handle it during render

                    Big issue.Resource State machine doesn't follows the attachments changes.... At the end of the pipeline we need to change it. The end doesn't matter.
                    For this a RenderPass object is neccessary.
                    Some thoughts about it :
                Why at the end ? During a render pass changing anything on the render target image with barriers does not makes sense.For that purpose one can use
                    subpasses and subpass dependeces
                    Why is it enough ? What if render pass and a download is recorded parallel ? It shouldn't happen. Between the two submit some synchronization is required. While it is
                    most probably happens on the same queue(or not? ) semaphores are used.Thus it needs to first submitted before it can be waited.Of course when these are timeline
                    semaphores theoretically the two commands could be recorded parallel.In this situation we can handle with a new objet some ScopedLocalTextureState.Which can be bypassed to the
                    state machine and handling locally the changes.In this case the texture shouldn't have a global state. That should be invalidated.
                    Add this node to the ADR.
                    Why wasn't it earlier a problem? It makes the things fishy only around the textures that are used as attachments and downloaded/uploaded.

                    To make it real we need a VulkanRenderPass.When it starts it needs a frame buffer.The frame buffer has the binding to a texture.So this is the point where we can access to the texture
                    and override its state.It is tricky because FrameBuffers are linked to the attachments.So when a VulkanFrameBuffer is attached to the VulkanRenderPass::begin() or the end() we need to change the corresponding
                    texture's state.

                    ResourceStateMachine::resetStages looks also fishy.If the same texture is used on a parallel rendering then it will cause a big issue ...
                    Resource states should be valid through a SubmitionScope :)
            */
            }
            bool isActive() const final { return true; }
            void registerExecutionContext(ExecutionContext&) final {};
        private:
            RefObj<OffscreenSwapChain> _swap_chain;
        };

        class OfflinePresentTask : public CpuNode::ICpuTask
        {
        public:
            explicit OfflinePresentTask(RefObj<OffscreenSwapChain> swap_chain)
                : _swap_chain(std::move(swap_chain))
            {}
            void run(CpuNode& self, ExecutionContext& execution_context) final
            {
                using namespace std::chrono_literals;
                const auto timeout = 1s;
                auto current_index = execution_context.getPoolIndex();
                _swap_chain->present(current_index.render_target_index,
                                     execution_context.getSyncObject(current_index.sync_object_index).getOperationsGroup(self.getName()));

            }
            bool isActive() const final { return true; }
            void registerExecutionContext(ExecutionContext&) final {};
        private:
            RefObj<OffscreenSwapChain> _swap_chain;
        };

        class OfflineReadbackTask : public CpuNode::ICpuTask
        {
        private:
            static ImageStream::ImageDescription createDescription(const OffscreenSwapChain& swap_chain)
            {
                const auto& image = swap_chain.getRenderTargetTextures().getTexture(0).getImage();
                return ImageStream::ImageDescription{
                    .width = image.getWidth(),
                    .height = image.getHeight(),
                    .format = image.getFormat()
                };
            }
        public:
            explicit OfflineReadbackTask(RefObj<OffscreenSwapChain> swap_chain)
                : _swap_chain(std::move(swap_chain))
                , _image_stream(createDescription(*_swap_chain))
            {}
            void run(CpuNode&, ExecutionContext& execution_context) final
            {
                using namespace std::chrono_literals;
                const auto timeout = 1s;
                auto current_index = execution_context.getPoolIndex();
                _swap_chain->readback(current_index.render_target_index,
                                      _image_stream);
                {
                    std::vector<uint8_t> data;
                    _image_stream >> data;
                    if (data.empty() == false && _saved_image_count < 10)
                    {
                        stbi_write_jpg(std::format("{:d}.jpg", _saved_image_count).c_str(),
                                       _image_stream.getImageDescription().width,
                                       _image_stream.getImageDescription().height,
                                       4,
                                       data.data(), 100);
                        _saved_image_count++;
                    }
                }
            }
            bool isActive() const final { return true; }
            void registerExecutionContext(ExecutionContext&) final {};
        private:
            RefObj<OffscreenSwapChain> _swap_chain;
            ImageStream _image_stream;
            uint32_t _saved_image_count{ 0 };
        };
    }
}



void ParallelDemoApplication::createRenderEngine()
{
    namespace rg = RenderEngine::RenderGraph;

    rg::RenderGraphBuilder builder = _render_engine->createRenderGraphBuilder("StandardPipeline");
    const rg::BinarySemaphore image_available_semaphore{ rg::ImageAcquireTask::image_available_semaphore_name };
    builder.registerSemaphore(image_available_semaphore);

    rg::ImageAcquireTask* image_acquire_task{ nullptr };
    {
        const auto base_render_target = _window->createRenderTarget();

        auto forward_renderer = std::make_unique<RenderEngine::ForwardRenderer>(*_render_engine.get(),
                                                                                base_render_target.clone()
                                                                                .changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                                                                                false);
        _forward_renderer = forward_renderer.get();

        auto ui_renderer = std::make_unique<RenderEngine::UIRenderer>(getUiWindow(),
                                                                      base_render_target.clone()
                                                                      .changeInitialLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                                                                      .changeLoadOperation(VK_ATTACHMENT_LOAD_OP_LOAD),
                                                                      3,
                                                                      false);
        _ui_renderer = ui_renderer.get();
        auto image_acquire_task_ptr = std::make_unique<rg::ImageAcquireTask>(getUiWindow(),
                                                                             getUiWindow().getDevice().getLogicalDevice(),
                                                                             _description.backbuffer_count,
                                                                             "ForwardRenderer");
        image_acquire_task = image_acquire_task_ptr.get();
        builder.addDeviceSynchronizeNode("SynchronizeRenderGpu", _window->getDevice());
        builder.addCpuNode("AcquireImage", std::move(image_acquire_task_ptr));
        builder.addRenderNode("ForwardRenderer", std::move(forward_renderer));
        builder.addRenderNode("ImguiRenderer", std::move(ui_renderer));
        builder.addPresentNode("Present", getUiWindow().getSwapChain());
        builder.addEmptyNode(RenderEngine::ParallelRenderEngine::kEndNodeName);
    }


    builder.addCpuSyncLink("AcquireImage", "ForwardRenderer")
        .signalOnGpu()
        .waitOnGpu(image_available_semaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuSyncLink("AcquireImage", "SynchronizeRenderGpu");
    builder.addCpuSyncLink("SynchronizeRenderGpu", "ForwardRenderer");
    /* TODO Needs to implement a merge tasks into the data transfer scheduler.
    * The data is transferred per resources (each transfer does a submit). Thus, it will signal the semaphore n times when n buffer is synchronized.
    .signalOnGpu(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
    .waitOnGpu(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    */
    builder.addCpuSyncLink("ForwardRenderer", "ImguiRenderer")
        .signalOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuSyncLink("ImguiRenderer", "Present")
        .signalOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    _render_engine->setRenderGraph(builder.reset("none"));

    _ui_renderer->setOnGui(
        [&]
        {
            onGui();
        });
}


void ParallelDemoApplication::createOffscreenRenderEngine()
{
    namespace rg = RenderEngine::RenderGraph;
    rg::RenderGraphBuilder builder = _render_engine->createRenderGraphBuilder("OffscreenPipeline");
    auto& device = _render_engine->getDevice();
    {
        auto base_render_target = _offscreen_textures->createRenderTarget();
        auto forward_renderer = std::make_unique<RenderEngine::ForwardRenderer>(*_render_engine.get(),
                                                                                base_render_target.clone()
                                                                                .changeFinalLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
                                                                                false);
        _forward_renderer = forward_renderer.get();
        auto image_acquire_task = std::make_unique<rg::OfflineImageAcquireTask>(*_offscreen_swap_chain);
        auto image_present_task = std::make_unique<rg::OfflinePresentTask>(*_offscreen_swap_chain);
        auto image_readback_task = std::make_unique<rg::OfflineReadbackTask>(*_offscreen_swap_chain);

        builder.addDeviceSynchronizeNode("SynchronizeRenderGpu", device);
        builder.addCpuNode("AcquireImage", std::move(image_acquire_task));
        builder.addRenderNode("ForwardRenderer", std::move(forward_renderer));
        builder.addCpuNode("Present", std::move(image_present_task));
        builder.addDeviceSynchronizeNode("SynchronizeReadBackImage", device);
        builder.addCpuNode("Readback", std::move(image_readback_task));
        builder.addEmptyNode(RenderEngine::ParallelRenderEngine::kEndNodeName);
    }

    builder.addCpuSyncLink("AcquireImage", "ForwardRenderer");
    builder.addCpuSyncLink("AcquireImage", "SynchronizeRenderGpu");
    builder.addCpuSyncLink("SynchronizeRenderGpu", "ForwardRenderer");
    /* TODO Needs to implement a merge tasks into the data transfer scheduler.
    * The data is transferred per resources (each transfer does a submit). Thus, it will signal the semaphore n times when n buffer is synchronized.
    .signalOnGpu(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
    .waitOnGpu(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    */
    builder.addCpuSyncLink("ForwardRenderer", "Present")
        .signalOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    builder.addCpuSyncLink("Present", "SynchronizeReadBackImage")
        .signalOnGpu(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    builder.addCpuSyncLink("SynchronizeReadBackImage", "Readback")
        .signalOnGpu(VK_PIPELINE_STAGE_2_TRANSFER_BIT)
        .waitOnGpu(VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    _render_engine->setRenderGraph(builder.reset("none"));

}

void ParallelDemoApplication::init()
{
    using namespace RenderEngine;

    initializeRenderers();
    createWindowSetup();
    if (_description.rendering_type == RenderingType::Direct)
    {
        createRenderEngine();
    }
    else
    {
        createOffscreenRenderEngine();
    }
    createScene();

    DemoSceneBuilder demoSceneBuilder;
    _scene_resources = demoSceneBuilder.buildSceneOfQuads(_assets,
                                                          *_scene,
                                                          _render_engine->getDevice(),
                                                          *_render_engine.get());

    ApplicationContext::instance().init(_scene.get(), _window != nullptr ? _window->getWindowHandle() : nullptr);

    _render_manager = std::make_unique<Scene::SceneRenderManager>(_scene->getNodeLookup(), _forward_renderer, nullptr);

    _render_manager->registerMeshesForRender();

    _asset_browser = std::make_unique<Ui::AssetBrowserUi>(_assets, *_scene);
}

void ParallelDemoApplication::createScene()
{
    Scene::Scene::SceneSetup scene_setup{
        .up = glm::vec3(0.0f, 1.0f, 0.0f),
        .forward = glm::vec3(0.0f, 0.0f, -1.0f)
    };

    _scene = std::make_unique<Scene::Scene>("Simple Scene", std::move(scene_setup));
}

void ParallelDemoApplication::run()
{
    while (_window == nullptr || _window->isClosed() == false)
    {
        ApplicationContext::instance().onFrameBegin();
        if (_window)
        {
            _window->update();
        }
        _render_engine->render();
        if (_present_render_engine.get() != nullptr)
        {
            _present_render_engine->render();
        }
        ApplicationContext::instance().updateInputEvents();
        ApplicationContext::instance().onFrameEnd();
    }
}

ParallelDemoApplication::~ParallelDemoApplication()
{
    if (_render_engine.get() != nullptr)
    {
        _render_engine->waitIdle();
    }
    if (_present_render_engine.get() != nullptr)
    {
        _present_render_engine->waitIdle();
    }
}
void ParallelDemoApplication::initializeRenderers()
{
    using namespace RenderEngine;

    std::unique_ptr<RendererFeactory> renderers = std::make_unique<RendererFeactory>();

    const bool enable_multi_device_selection = _description.rendering_type == RenderingType::OffscreenWithPresent;
    DeviceSelector device_selector(enable_multi_device_selection);
    RenderContext::InitializationInfo init_info{};
    init_info.app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    init_info.app_info.apiVersion = VK_API_VERSION_1_3;
    init_info.app_info.applicationVersion = 0;
    init_info.app_info.engineVersion = 0;
    init_info.app_info.pApplicationName = "DemoApplication";
    init_info.app_info.pEngineName = "FoxEngine";
    init_info.enable_validation_layers = true;
    init_info.enabled_layers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_NV_optimus" };
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
    switch (_description.rendering_type)
    {
        case RenderingType::Direct:
        {
            auto render_engine = RenderContext::context().getDevice(_present_device_index).createParallelRenderEngine(_description.backbuffer_count, _description.thread_count);
            _render_engine.reset(render_engine.get());
            _window = RenderContext::context().getDevice(_present_device_index).createParallelWindow(_description.window_title,
                                                                                                     _description.backbuffer_count,
                                                                                                     std::move(render_engine));
        }
        break;
        case RenderingType::Offscreen:
        {
            auto render_engine = RenderContext::context().getDevice(_render_device_index).createParallelRenderEngine(_description.backbuffer_count, _description.thread_count);
            _render_engine.reset(std::move(render_engine));

            createOffscreenWindow();
        }
        break;
        case RenderingType::OffscreenWithPresent:
        {
            {
                auto render_engine = RenderContext::context().getDevice(_render_device_index).createParallelRenderEngine(_description.backbuffer_count, _description.thread_count);
                _render_engine.reset(std::move(render_engine));
                createOffscreenWindow();
            }
            {
                auto render_engine = RenderContext::context().getDevice(_present_device_index).createParallelRenderEngine(_description.backbuffer_count, _description.thread_count);
                _present_render_engine.reset(render_engine.get());
                _window = RenderContext::context().getDevice(_present_device_index).createParallelWindow(_description.window_title,
                                                                                                         _description.backbuffer_count,
                                                                                                         std::move(render_engine));
            }
        }
    }

}

void ParallelDemoApplication::createOffscreenWindow()
{
    auto& device = RenderEngine::RenderContext::context().getDevice(_render_device_index);
    RenderEngine::Image render_target_image(_description.window_width, _description.window_height, VK_FORMAT_R8G8B8A8_SRGB);
    std::vector<std::unique_ptr<RenderEngine::Texture>> render_target_textures;
    for (uint32_t i = 0; i < _description.backbuffer_count; ++i)
    {
        render_target_textures.push_back(device.getTextureFactory().createNoUpload(render_target_image,
                                                                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                                                                   VK_SHADER_STAGE_ALL,
                                                                                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));
    }

    _offscreen_textures = std::make_unique<RenderEngine::RenderTargetTextures>(std::move(render_target_textures));
    _offscreen_swap_chain = std::make_unique<RenderEngine::OffscreenSwapChain>(device, *_offscreen_textures);
}
