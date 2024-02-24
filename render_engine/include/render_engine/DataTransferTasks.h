#pragma once

#include <render_engine/synchronization/SyncObject.h>
#include <render_engine/synchronization/SyncOperations.h>
#include <render_engine/TransferEngine.h>

#include <functional>
#include <vector>

namespace RenderEngine
{
    class Image;
    class Texture;

    class StartToken
    {
        StartToken() = default;
        friend class DataTransferScheduler;
    };
    class UploadTask
    {
    public:
        class Storage
        {
        public:
            void storeStagingData(VkBuffer buffer, VkDeviceMemory memory, LogicalDevice* logical_device);
            ~Storage();
        private:
            VkBuffer _staging_buffer{ VK_NULL_HANDLE };
            VkDeviceMemory _staging_memory{ VK_NULL_HANDLE };
            LogicalDevice* _logical_device{ nullptr };
        };
        explicit UploadTask(std::function<std::vector<SyncObject>(SyncOperations sync_operations, TransferEngine& transfer_engine, Storage&)>&& task)
            : _task(std::move(task))
        {}

        UploadTask(UploadTask&&) = default;
        UploadTask(const UploadTask&) = default;

        UploadTask& operator=(UploadTask&&) = default;
        UploadTask& operator=(const UploadTask&) = default;

        bool isStarted() const;
        SyncOperations getSyncOperations() const;
        void start(StartToken, SyncOperations in_operations, TransferEngine& transfer_engine);
        bool isFinished();
    private:
        std::function<std::vector<SyncObject>(SyncOperations sync_operations, TransferEngine& transfer_engine, Storage&)> _task;
        bool _started{ false };
        std::vector<SyncObject> _transfer_objects;
        Storage _storage;
    };

    class DownloadTask
    {
    public:
        DownloadTask(std::function<std::vector<SyncObject>(SyncOperations sync_operations, TransferEngine& transfer_engine)>&& task,
                     Texture* texture)
            : _task(std::move(task))
            , _texture(texture)
        {}

        DownloadTask(DownloadTask&&) = default;
        DownloadTask(const DownloadTask&) = delete;

        DownloadTask& operator=(DownloadTask&&) = default;
        DownloadTask& operator=(const DownloadTask&) = delete;
        bool isFinished();

        bool isStarted() const;
        Image getImage();
        SyncOperations getSyncOperations() const;
        void start(StartToken, SyncOperations in_operations, TransferEngine& transfer_engine);

    private:
        std::function<std::vector<SyncObject>(SyncOperations sync_operations, TransferEngine& transfer_engine)> _task;
        bool _started{ false };
        std::vector<SyncObject> _transfer_objects;
        Texture* _texture{ nullptr };
    };

}