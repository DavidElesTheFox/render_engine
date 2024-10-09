#include <render_engine/rendergraph/cputasks/ImageAquireTask.h>

#include <render_engine/debug/Profiler.h>
#include <render_engine/RenderContext.h>
#include <render_engine/rendergraph/Topic.h>

namespace RenderEngine::RenderGraph
{
    static thread_local std::string g_thread_name = []
        {
            const std::thread::id id = std::this_thread::get_id();
            return std::format("Thread-{:}", *reinterpret_cast<const unsigned int*>(&id));
        }();

    class ImageAcquireTask::SwapChainImageSelector
    {
    public:
        SwapChainImageSelector(uint32_t image_count,
                               LogicalDevice& logical_device,
                               SwapChain& swap_chain)
            : _image_count(image_count)
            , _logical_device(logical_device)
            , _swap_chain(swap_chain)
        {}
        /**
          * @brief Determines the next available image index.
          *
          * This is tricky:
          * There are renders ongoing on the back buffers. Each back buffer has its own semaphore to trigger when the drawing are finished
          * Its gonna be the available_index. Its semaphore will be used to query the next back buffer.
          *
         */
        std::optional<ExecutionContext::PoolIndex> getNextImage(ExecutionContext& execution_context);

        void releasePoolIndex(const ExecutionContext::PoolIndex& index)
        {
            auto lock = std::unique_lock(_occupied_indexes_mutex);
            _occupied_sync_object_indexes.erase(index.sync_object_index);
        }
    private:

        void markSyncIndexAsOccupied(uint32_t sync_index)
        {
            std::unique_lock lock{ _occupied_indexes_mutex };
            _occupied_sync_object_indexes.insert(sync_index);
        }
        // TODO remove events. Not used anymore
        IndexSet<uint32_t> getOccupiedSyncIndexes() const
        {
            std::shared_lock lock(_occupied_indexes_mutex);
            return _occupied_sync_object_indexes;
        }

        uint32_t chooseSyncObjectIndex(const std::vector<const SyncObject*>& sync_objects);

        std::optional<uint32_t> tryAcquireImage(ExecutionContext& execution_context) const;

        mutable std::shared_mutex _occupied_indexes_mutex;
        IndexSet<uint32_t> _occupied_sync_object_indexes; /**< On the CPU the draw calls are recording right now*/

        uint32_t _image_count{ 0 };
        LogicalDevice& _logical_device;
        SwapChain& _swap_chain;
        std::mutex _index_selection_mutex;

    };


    std::optional<ExecutionContext::PoolIndex> ImageAcquireTask::SwapChainImageSelector::getNextImage(ExecutionContext& execution_context)
    {
        PROFILE_SCOPE();

        std::optional<uint32_t> image_index = tryAcquireImage(execution_context);

        if (image_index == std::nullopt)
        {
            return std::nullopt;
        }


        RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphExecution{},
                                                     "Image Acquire: Image index {:d} frame: {:d} (Using synchronization object: {:d}) [thread: {}]",
                                                     *image_index,
                                                     execution_context.getCurrentFrameNumber(),
                                                     execution_context.getPoolIndex().sync_object_index,
                                                     (std::stringstream{} << std::this_thread::get_id()).str());

        return ExecutionContext::PoolIndex{ .render_target_index = *image_index, .sync_object_index = execution_context.getPoolIndex().sync_object_index };
    }
    uint32_t ImageAcquireTask::SwapChainImageSelector::chooseSyncObjectIndex(const std::vector<const SyncObject*>& sync_objects)
    {
        PROFILE_SCOPE();
        std::unique_lock lock(_occupied_indexes_mutex);
        const uint32_t available_index = [&]
            {
                for (uint32_t i = 0; i < sync_objects.size(); ++i)
                {
                    if (_occupied_sync_object_indexes.contains(i) == false)
                    {
                        return i;
                    }
                }
                assert(false && "Couldn't find any available item in the pool");
                return 0u;
            }();
        _occupied_sync_object_indexes.insert(available_index);
        return available_index;
    }
    std::optional<uint32_t> ImageAcquireTask::SwapChainImageSelector::tryAcquireImage(ExecutionContext& execution_context) const
    {
        PROFILE_SCOPE();
        using namespace std::chrono_literals;
        const auto timeout = 1ms;
        const auto wait_time = 1ms;

        SwapChain::ImageAcquireResult acquire_result = {
            .image_index = 0,
            .result = VK_TIMEOUT
        };

        /*
        * Because forward progress is not guaranteed with this setup we need to specify a timeout.
        * I.e.: We are requesting 'too' much image from the swap chain.
        * See more details:
        * https://vulkan.lunarg.com/doc/view/1.3.275.0/windows/1.3-extensions/vkspec.html#VUID-vkAcquireNextImageKHR-surface-07783
        * https://vulkan.lunarg.com/doc/view/1.3.275.0/windows/1.3-extensions/vkspec.html#swapchain-acquire-forward-progress
        */
        while (acquire_result.result == VK_TIMEOUT)
        {
            PROFILE_SCOPE("SwapChainImageSelector::tryAcquireImage - probe");
            const uint64_t timeout_ns = timeout.count();
            acquire_result = _swap_chain.acquireNextImage(timeout_ns,
                                                          execution_context.getSyncObject(execution_context.getPoolIndex().sync_object_index)
                                                          .getPrimitives().getSemaphore(ImageAcquireTask::image_available_semaphore_name),
                                                          VK_NULL_HANDLE);
            if (acquire_result.result == VK_TIMEOUT)
            {
                PROFILE_SCOPE("SwapChainImageSelector::tryAcquireImage - sleep");

                // std::this_thread::sleep_for(wait_time);
            }
        }
        switch (acquire_result.result)
        {
            case VK_ERROR_OUT_OF_DATE_KHR:
            case VK_SUBOPTIMAL_KHR:
                // TODO replace it with an exception, while it is an exceptional case. Makes much simpler the interface
                return std::nullopt;
            case VK_SUCCESS:
                break;
            default:
                throw std::runtime_error("Failed to query swap chain image");
        }
        return acquire_result.image_index;
    }


    ImageAcquireTask::ImageAcquireTask(Window& window,
                                       LogicalDevice& logical_device,
                                       uint32_t back_buffer_count,
                                       std::string image_user_node_name)
        : _window(window)
        , _swap_chain_image_selector(std::make_unique<SwapChainImageSelector>(back_buffer_count, logical_device, window.getSwapChain()))
        , _image_user_node_name(std::move(image_user_node_name))
    {}

    ImageAcquireTask::~ImageAcquireTask() = default;

    void ImageAcquireTask::run(CpuNode&, ExecutionContext& execution_context)
    {
        PROFILE_THREAD(g_thread_name.c_str());
        PROFILE_SCOPE();
        /*
        * When we are here we know that the process is finished. It means that the draw calls were issued on Host.
        * But in order to use the semaphore we need to wait until it has no more ongoing work on the GPU.
        * It means we need to wait until the execution is finished on the GPU:
        */
        auto sync_object_index = execution_context.getPoolIndex().sync_object_index;
        execution_context.getSyncFeedbackService().get(&execution_context.getSyncObject(sync_object_index), _image_user_node_name)->wait();

        std::optional<ExecutionContext::PoolIndex> pool_index = _swap_chain_image_selector->getNextImage(execution_context);

        if (pool_index == std::nullopt)
        {
            _window.reinitSwapChain();
        }
        else
        {
            execution_context.setPoolIndex(*pool_index);
        }

    }
    void ImageAcquireTask::registerExecutionContext(ExecutionContext& execution_context)
    {
        PROFILE_SCOPE();
        ExecutionContext::Events events
        {
            .on_pool_index_set = nullptr,
            .on_pool_index_clear = [this](const auto& index) { _swap_chain_image_selector->releasePoolIndex(index); }
        };
        execution_context.addEvents(std::move(events));
    }
}