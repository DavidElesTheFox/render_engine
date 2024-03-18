#include <render_engine/DataTransferTasks.h>

#include <render_engine/assets/Image.h>
#include <render_engine/DataTransferScheduler.h>
#include <render_engine/resources/Texture.h>

#include <cassert>
namespace RenderEngine
{
    UploadTask::Storage::~Storage()
    {
        if (_logical_device)
        {
            auto& logical_device = *_logical_device;
            logical_device->vkDestroyBuffer(*logical_device, _staging_buffer, nullptr);
            logical_device->vkFreeMemory(*logical_device, _staging_memory, nullptr);
        }
    }

    UploadTask::~UploadTask()
    {
        _submit_tracker->wait();
    }

    void UploadTask::Storage::storeStagingData(VkBuffer buffer, VkDeviceMemory memory, LogicalDevice* logical_device)
    {
        _logical_device = logical_device;
        _staging_buffer = buffer;
        _staging_memory = memory;
    }

    bool UploadTask::isStarted() const
    {
        return _started;
    }
    void UploadTask::wait()
    {
        _submit_tracker->wait();
    }


    void UploadTask::start(StartToken, SyncOperations in_operations, TransferEngine& transfer_engine)
    {
        _transfer_objects = _task(in_operations, transfer_engine, _storage, *_submit_tracker);
        assert(_transfer_objects.empty() == false && "transfer must return at leas one sync objects to be able to wait for its finish");
        assert(_transfer_objects.back().getPrimitives().hasTimelineSemaphore(DataTransferScheduler::kDataTransferFinishSemaphoreName)
               && "The last object needs to have the timeline semaphore that can be waited");
    }

    SyncOperations UploadTask::getSyncOperations() const
    {
        // TODO: Implement multi threaded upload
        assert(isStarted() && "Until the engine is not multi threaded the operation needs to be started before it is waited.");
        return _transfer_objects.back().getOperationsGroup(SyncGroups::kExternal);
    }

    DownloadTask::~DownloadTask()
    {
        _submit_tracker->wait();
    }

    bool DownloadTask::isStarted() const
    {
        return _started;
    }

    bool UploadTask::isFinished()
    {
        return _transfer_objects.back().getSemaphoreValue(DataTransferScheduler::kDataTransferFinishSemaphoreName) == 1;
    }

    void DownloadTask::start(StartToken, SyncOperations in_operations, TransferEngine& transfer_engine)
    {
        _transfer_objects = _task(in_operations, transfer_engine, *_submit_tracker);
        assert(_transfer_objects.empty() == false && "transfer must return at leas one sync objects to be able to wait for its finish");
        assert(_transfer_objects.back().getPrimitives().hasTimelineSemaphore(DataTransferScheduler::kDataTransferFinishSemaphoreName)
               && "The last object needs to have the timeline semaphore that can be waited");
    }

    SyncOperations DownloadTask::getSyncOperations() const
    {
        // TODO: Implement multi threaded upload
        assert(isStarted() && "Until the engine is not multi threaded the operation needs to be started before it is waited.");
        return _transfer_objects.back().getOperationsGroup(SyncGroups::kExternal);
    }

    Image DownloadTask::getImage()
    {
        _transfer_objects.back().waitSemaphore(DataTransferScheduler::kDataTransferFinishSemaphoreName, 1);
        std::vector<uint8_t> data(_texture->getStagingBuffer().getDeviceSize());
        const uint8_t* staging_memory = static_cast<const uint8_t*>(_texture->getStagingBuffer().getMemory());
        std::copy(staging_memory,
                  staging_memory + data.size(), data.data());
        Image result = _texture->getImage();
        result.setData(std::move(data));
        return result;
    }

    bool DownloadTask::isFinished()
    {
        return _transfer_objects.back().getSemaphoreValue(DataTransferScheduler::kDataTransferFinishSemaphoreName) == 1;
    }
}