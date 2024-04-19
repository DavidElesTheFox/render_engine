#pragma once

#include <render_engine/RenderContext.h>
#include <render_engine/rendergraph/DependentName.h>
#include <render_engine/rendergraph/Graph.h>
#include <render_engine/rendergraph/Link.h>

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <variant>

#include <volk.h>

namespace RenderEngine
{
    namespace RenderGraph
    {
        class RenderGraphBuilder
        {
        public:

            class BinarySemaphore
            {
            public:
                explicit BinarySemaphore(RenderGraph::DependentName name);
                const RenderGraph::DependentName& getName() const;
            private:
                RenderGraph::DependentName _name;
            };

            class TimelineSemaphore
            {
            public:
                TimelineSemaphore(RenderGraph::DependentName name, uint64_t initial_value, uint64_t value_range);
                const RenderGraph::DependentName& getName() const;
                uint64_t getInitialValue() const;
                uint64_t getValueRange() const;

            private:
                RenderGraph::DependentName _name;
                uint64_t _initial_value{ 0 };
                uint64_t _value_range{ 0 };
            };

            template<uint32_t OperationFlag = 0>
            class GpuLinkBuilder
            {
            private:
                using EmptySignalData = std::tuple<>;
                using BinarySemaphoreSignalData = std::tuple<BinarySemaphore, VkPipelineStageFlagBits2>;
                using TimelineSemaphoreSignalData = std::tuple<TimelineSemaphore, uint64_t, VkPipelineStageFlagBits2>;
            public:
                enum
                {
                    OP_FLAG_NO = 0,
                    OP_FLAG_SIGNAL = 1,
                    OP_FLAG_WAIT = 1 << 1,
                    OP_FLAG_WAIT_EXTERNAL = 1 << 2,
                    OP_FLAG_ALL = (1 << 3) - 1
                };
                explicit GpuLinkBuilder(RenderGraph::Link& link)
                    : _link(link)
                {}

                template<uint32_t OtherFlag>
                GpuLinkBuilder(GpuLinkBuilder<OtherFlag>&& o)
                    : _link(link)
                    , _from_node(std::move(o._from_node))
                    , _to_node(std::move(o._to_node))
                    , _signal_data(std::move(o._signal_data))
                {

                }
                template<std::enable_if_t<OperationFlag& GpuLinkBuilder::OP_FLAG_SIGNAL != 0, bool> = true>
                GpuLinkBuilder<OP_FLAG_WAIT_EXTERNAL>&& signalOnGpu()&&
                {
                    return { std::move(*this) };
                }
                template<std::enable_if_t<OperationFlag& GpuLinkBuilder::OP_FLAG_SIGNAL != 0, bool> = true>
                GpuLinkBuilder<OP_FLAG_WAIT>&& signalOnGpu(VkPipelineStageFlagBits2 signal_mask)&&
                {
                    _signal_data = std::make_tuple(BinarySemaphore{ generateSamaphoreName() }, signal_mask);
                    return { std::move(*this) };
                }
                template<std::enable_if_t<OperationFlag& GpuLinkBuilder::OP_FLAG_SIGNAL != 0, bool> = true>
                GpuLinkBuilder<OP_FLAG_WAIT>&& signalOnGpu(const TimelineSemaphore& semaphore, uint64_t value, VkPipelineStageFlagBits2 signal_mask)&&
                {
                    _signal_data = std::make_tuple(semaphore, value, signal_mask);
                    return { std::move(*this) };

                }
                template<std::enable_if_t<OperationFlag& GpuLinkBuilder::OP_FLAG_SIGNAL != 0, bool> = true>
                GpuLinkBuilder<OP_FLAG_WAIT>&& signalOnGpu(const BinarySemaphore& semaphore, VkPipelineStageFlagBits2 signal_mask)&&
                {
                    _signal_data = std::make_tuple(semaphore, signal_mask);
                    return { std::move(*this) };
                }

                template<std::enable_if_t<OperationFlag& GpuLinkBuilder::OP_FLAG_WAIT != 0, bool> = true>
                GpuLinkBuilder<OP_FLAG_NO>&& waitOnGpu(VkPipelineStageFlagBits2 wait_mask)&&
                {
                    using ExternalConnection = RenderGraph::Link::ExternalConnection;
                    using PipelineConnection = RenderGraph::Link::PipelineConnection;
                    std::visit(overloaded(
                        [&](const EmptySignalData&) { assert(false && "Operation is not allowed"); }
                    [&](const BinarySemaphoreSignalData& data) { _link.addConnection(PipelineConnection(std::get<BinarySemaphore>(data).getName(), std::get<VkPipelineStageFlagBits2>(data), wait_mask)); }
                    [&](const TimelineSemaphoreSignalData& data) { _link.addConnection(PipelineConnection(std::get<BinarySemaphore>(data).getName(), std::get<VkPipelineStageFlagBits2>(data), wait_mask, std::get<uint64_t>(data))); }
                    ), _signal_data);
                    return { std::move(*this) };
                }
                template<std::enable_if_t<OperationFlag& GpuLinkBuilder::OP_FLAG_WAIT_EXTERNAL != 0, bool> = true>
                GpuLinkBuilder<OP_FLAG_NO>&& waitOnGpu(BinarySemaphore semaphore, VkPipelineStageFlagBits2 wait_mask)&&
                {
                    using ExternalConnection = RenderGraph::Link::ExternalConnection;
                    using PipelineConnection = RenderGraph::Link::PipelineConnection;
                    std::visit(overloaded(
                        [&](const EmptySignalData& data) { _link.addConnection(ExternalConnection(semaphore.getName(), wait_mask)); }
                    [&](const BinarySemaphoreSignalData&) { assert(false && "Operation is not allowed"); }
                    [&](const TimelineSemaphoreData&) { assert(false && "Operation is not allowed"); }
                    ), _signal_data);
                    return { std::move(*this) };
                }
                template<std::enable_if_t<OperationFlag& GpuLinkBuilder::OP_FLAG_WAIT_EXTERNAL != 0, bool> = true>
                GpuLinkBuilder<OP_FLAG_NO>&& waitOnGpu(TimelineSemaphore semaphore, int64_t value, VkPipelineStageFlagBits2 wait_mask)&&
                {
                    using ExternalConnection = RenderGraph::Link::ExternalConnection;
                    using PipelineConnection = RenderGraph::Link::PipelineConnection;
                    std::visit(overloaded(
                        [&](const EmptySignalData& data) { _link.addConnection(ExternalConnection(semaphore.getName(), wait_mask, value)); }
                    [&](const BinarySemaphoreSignalData&) { assert(false && "Operation is not allowed"); }
                    [&](const TimelineSemaphoreData&) { assert(false && "Operation is not allowed"); }
                    ), _signal_data);
                    return { std::move(*this) };
                }
            private:
                RenderGraph::DependentName generateSemaphoreName() const
                {
                    return RenderGraph::DependentName::fromString(_from_name + "-" + _to_name + "-{:d}");
                }

                Link& _link;
                std::string _from_name;
                std::string _to_name;

                std::variant<EmptySignalData, BinarySemaphoreSignalData, TimelineSemaphoreSignalData> _signal_data;
            };
            using GeneralGpuLinkBuilder = GpuLinkBuilder<GpuLinkBuilder<>::OP_FLAG_ALL>;
            explicit RenderGraphBuilder(std::string name,
                                        std::weak_ptr<CommandContext> render_context,
                                        std::weak_ptr<CommandContext> present_context,
                                        std::weak_ptr<SingleShotCommandContext> transfer_context);

            ~RenderGraphBuilder() = default;

            void addRenderNode(std::string name, std::unique_ptr<AbstractRenderer> renderer);
            void addTransferNode(std::string name);
            void addComputeNode(std::string name, std::unique_ptr<RenderGraph::ComputeNode::IComputeTask> task);
            void addCpuNode(std::string name, std::unique_ptr<RenderGraph::CpuNode::ICpuTask> task);
            void addPresentNode(std::string name, SwapChain& swap_chain);
            void addEmptyNode(std::string name);

            GeneralGpuLinkBuilder addCpuSyncLink(const std::string& from, const std::string& to);
            GeneralGpuLinkBuilder addCpuAsyncLink(const std::string& from, const std::string& to);

            void registerSemaphore(BinarySemaphore semaphore);
            void registerSemaphore(TimelineSemaphore semaphore);

            std::unique_ptr<RenderGraph::Graph> reset(std::string new_name)
            {
                auto result = std::move(_graph);
                _graph = std::make_unique<RenderGraph::Graph>(std::move(new_name));
                return result;
            }
        private:

            std::unique_ptr<RenderGraph::Graph> _graph;
            std::weak_ptr<CommandContext> _render_context;
            std::weak_ptr<CommandContext> _present_context;
            std::weak_ptr<SingleShotCommandContext> _transfer_context;
        };
    }
}