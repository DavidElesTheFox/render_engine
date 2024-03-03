#pragma once

#include <render_engine/synchronization/SyncOperations.h>

#include <future>
#include <span>
#include <unordered_set>
namespace RenderEngine
{
    class Buffer;
    class Texture;
    struct TextureState;
    struct BufferState;
    class Image;
    class UploadTask;
    class DownloadTask;
    class CommandContext;
    class SyncOperations;
    class TransferEngine;

    class DataTransferScheduler
    {
    public:
        static const std::string kDataTransferFinishSemaphoreName;

        ~DataTransferScheduler();

        // TODO: Remove final parameter. RenderGraph should be control that dependency
        std::weak_ptr<UploadTask> upload(Texture* texture,
                                         Image image,
                                         SingleShotCommandContext& dst_context,
                                         TextureState final_state,
                                         SyncOperations sync_operations = {});
        std::weak_ptr<UploadTask> upload(Buffer* buffer,
                                         std::vector<uint8_t> data,
                                         SingleShotCommandContext& dst_context,
                                         BufferState final_state);

        std::weak_ptr<UploadTask> upload(Buffer* buffer,
                                         std::span<const uint8_t> data,
                                         SingleShotCommandContext& dst_context,
                                         BufferState final_state);
        template<typename T>
        std::weak_ptr<UploadTask> upload(Buffer* buffer,
                                         std::span<T> data,
                                         SingleShotCommandContext& dst_context,
                                         BufferState final_state)
        {
            return upload(buffer,
                          std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), sizeof(T) * data.size()),
                          dst_context,
                          std::move(final_state));

        }
        std::weak_ptr<DownloadTask> download(Texture* texture,
                                             SyncOperations sync_operations = {});

        void executeTasks(SyncOperations sync_operations, TransferEngine& transfer_engine);
        bool hasAnyTask() const;
    private:
        /* TODO: Batch upload/download
        * Instead of storing the tasks it would be enough to store the input parameters.
        * And than execute the data transfer in one submit for multiple texture/buffer.
        * It results that multiple texture gonna have a reference to only one task. But it shouldn't be an issue.
        */
        template<typename T>
        struct StagingAera
        {
            std::unordered_map<T*, std::shared_ptr<UploadTask>> uploads;
            std::unordered_map<T*, std::shared_ptr<DownloadTask>> downloads;
        };

        std::mutex _task_mutex;
        StagingAera<Buffer> _buffers_staging_area;
        StagingAera<Texture> _textures_staging_area;
    };
}