#pragma once

#include <render_engine/rendergraph/DependentName.h>
#include <render_engine/synchronization/SyncObject.h>

#include <functional>
#include <variant>
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
        struct PipelineConnection
        {
            PipelineConnection(std::string semaphore_name,
                               VkPipelineStageFlags2 signal_stage,
                               VkPipelineStageFlags2 wait_stage,
                               std::optional<uint64_t> value);
            std::string semaphore_name;
            VkPipelineStageFlags2 signal_stage{ VK_PIPELINE_STAGE_2_NONE };
            VkPipelineStageFlagBits2 wait_stage{ VK_PIPELINE_STAGE_2_NONE };
            std::optional<uint64_t> value{ 0 };
        };
        struct ExternalConnection
        {

            ExternalConnection(std::string signaled_semaphore_name,
                               VkPipelineStageFlags2 wait_stage,
                               std::optional<uint64_t> value);
            std::string signaled_semaphore_name;
            VkPipelineStageFlagBits2 wait_stage{ VK_PIPELINE_STAGE_2_NONE };
            std::optional<uint64_t> value{ 0 };
        };

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

        void addConnection(PipelineConnection connection)
        {
            _connections.emplace_back(std::move(connection));
        }
        void addConnection(ExternalConnection connection)
        {
            _connections.emplace_back(std::move(connection));
        }

        void forEachConnections(const std::function<void(const PipelineConnection&)>& callback_pipeline,
                                const std::function<void(const ExternalConnection&)>& callback_external) const;

    private:

        LinkType _type{ LinkType::Unknown };
        const Node* _from_node{ nullptr };
        const Node* _to_node{ nullptr };
        std::vector<std::variant<PipelineConnection, ExternalConnection>> _connections;
    };
}