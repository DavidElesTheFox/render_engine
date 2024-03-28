#pragma once

#include <render_engine/synchronization/SyncObject.h>

#include <format>

namespace RenderEngine::RenderGraph
{
    class Node;
    class GraphVisitor;

    enum class LinkType
    {
        Unknown = 0,
        CpuSync,
        CpuAsync
    };

    class Link final
    {
    public:
        static std::string syncGroup(uint32_t render_target_index) { return std::vformat("group:{:d}", std::make_format_args(render_target_index)); }
        Link(const Node* from, const Node* to, LinkType type)
            : _from_node(from)
            , _to_node(to)
            , _type(type)
        {}

        Link(Link&&) = delete;
        Link(const Link&) = delete;

        Link& operator=(Link&&) = delete;
        Link& operator=(const Link&) = delete;

        void accept(GraphVisitor&);

        LinkType getType() const { return _type; }
        const Node* getFromNode() const { return _from_node; }
        const Node* getToNode() const { return _to_node; }

        void addConnection(VkPipelineStageFlags2 signal_stage, VkPipelineStageFlags2 wait_stage)
        {
            _connections.emplace_back(signal_stage, wait_stage);
        }
        void addConnection(const std::string& signaled_semaphore_name, VkPipelineStageFlags2 wait_stage)
        {
            _connections.emplace_back(wait_stage);
        }

        void addSyncOperationsForSource(SyncObject& sync_object, uint32_t back_buffer_size) const;
        void addSyncOperationsForDestination(SyncObject& sync_object, uint32_t back_buffer_size) const;

    private:
        struct Connection
        {
            Connection(VkPipelineStageFlags2 signal_stage,
                       VkPipelineStageFlags2 wait_stage)
                : signal_stage(signal_stage)
                , signaled_semaphore_name(std::nullopt)
                , wait_stage(wait_stage)
            {}
            Connection(const std::string& signaled_semaphore_name,
                       VkPipelineStageFlags2 wait_stage)
                : signal_stage(std::nullopt)
                , signaled_semaphore_name(signaled_semaphore_name)
                , wait_stage(wait_stage)
            {}
            std::optional<VkPipelineStageFlags2> signal_stage{ VK_PIPELINE_STAGE_2_NONE };
            std::optional<std::string> signaled_semaphore_name;
            VkPipelineStageFlagBits2 wait_stage{ VK_PIPELINE_STAGE_2_NONE };
        };
        LinkType _type{ LinkType::Unknown };
        const Node* _from_node{ nullptr };
        const Node* _to_node{ nullptr };
        std::vector<Connection> _connections;
    };
}