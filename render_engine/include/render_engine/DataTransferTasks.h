#pragma once

#include <render_engine/QueueSubmitTracker.h>
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
        explicit UploadTask(LogicalDevice& logical_device,
                            std::function<std::vector<SyncObject>(SyncOperations, TransferEngine&, Storage&, QueueSubmitTracker&)>&& task,
                            std::string name)
            : _task(std::move(task))
            , _submit_tracker(std::make_unique<QueueSubmitTracker>(logical_device, std::move(name)))
        {}

        ~UploadTask();

        UploadTask(UploadTask&&) = default;
        UploadTask(const UploadTask&) = default;

        UploadTask& operator=(UploadTask&&) = default;
        UploadTask& operator=(const UploadTask&) = default;

        bool isStarted() const;
        SyncOperations getSyncOperations() const;
        void start(StartToken, SyncOperations in_operations, TransferEngine& transfer_engine);
        bool isFinished();
        void wait();
    private:
        std::function<std::vector<SyncObject>(SyncOperations sync_operations, TransferEngine& transfer_engine, Storage&, QueueSubmitTracker&)> _task;
        bool _started{ false };
        std::vector<SyncObject> _transfer_objects;
        Storage _storage;
        std::unique_ptr<QueueSubmitTracker> _submit_tracker;
    };

    class DownloadTask
    {
    public:
        DownloadTask(std::function<std::vector<SyncObject>(SyncOperations sync_operations,
                                                           TransferEngine& transfer_engine,
                                                           QueueSubmitTracker& submit_tracker)>&& task,
                     Texture* texture,
                     LogicalDevice& logical_device,
                     std::string name)
            : _task(std::move(task))
            , _texture(texture)
            , _submit_tracker(std::make_unique<QueueSubmitTracker>(logical_device, std::move(name)))
        {}
        ~DownloadTask();

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
        std::function<std::vector<SyncObject>(SyncOperations, TransferEngine&, QueueSubmitTracker&)> _task;
        bool _started{ false };
        std::vector<SyncObject> _transfer_objects;
        Texture* _texture{ nullptr };
        std::unique_ptr<QueueSubmitTracker> _submit_tracker;
    };

}