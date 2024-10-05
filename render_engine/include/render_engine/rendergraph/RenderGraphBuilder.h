#pragma once

#include <render_engine/containers/VariantOverloaded.h>
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
            enum
            {
                OP_FLAG_NOP = 0,
                OP_FLAG_SIGNAL = 1,
                OP_FLAG_WAIT = 1 << 1,
                OP_FLAG_WAIT_EXTERNAL = 1 << 2
            };

            template<uint32_t OperationFlag>
            class GpuLinkBuilder;

            using GeneralGpuLinkBuilder = GpuLinkBuilder<OP_FLAG_SIGNAL>;

            explicit RenderGraphBuilder(std::string name,
                                        CommandBufferContext render_context,
                                        CommandBufferContext present_context,
                                        TransferEngine transfer_engine);

            ~RenderGraphBuilder() = default;

            void addRenderNode(std::string name, std::unique_ptr<AbstractRenderer> renderer);
            void addTransferNode(std::string name);
            void addDeviceSynchronizeNode(std::string name, Device& device);
            void addComputeNode(std::string name, std::unique_ptr<RenderGraph::ComputeNode::IComputeTask> task);
            void addCpuNode(std::string name, std::unique_ptr<RenderGraph::CpuNode::ICpuTask> task);
            void addPresentNode(std::string name, SwapChain& swap_chain);
            void addEmptyNode(std::string name);

            GeneralGpuLinkBuilder addCpuSyncLink(const std::string& from, const std::string& to);
            GeneralGpuLinkBuilder addCpuAsyncLink(const std::string& from, const std::string& to);

            void registerSemaphore(BinarySemaphore semaphore) { _graph->registerSemaphore(std::move(semaphore)); }
            void registerSemaphore(TimelineSemaphore semaphore) { _graph->registerSemaphore(std::move(semaphore)); };

            std::unique_ptr<RenderGraph::Graph> reset(std::string new_name)
            {
                auto result = std::move(_graph);
                result->applyChanges();
                _graph = std::make_unique<RenderGraph::Graph>(std::move(new_name));
                return result;
            }
        private:

            std::unique_ptr<RenderGraph::Graph> _graph;
            CommandBufferContext _render_context;
            CommandBufferContext _present_context;
            TransferEngine _transfer_engine;

        };

        template<>
        class RenderGraphBuilder::GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>
        {
        private:
            using EmptySignalData = std::tuple<>;
            using BinarySemaphoreSignalData = std::tuple<BinarySemaphore, VkPipelineStageFlagBits2>;
            // VKPipelineStageFlagBits is an uint64_t. Need another type for tuple
            struct SignalValue
            {
                uint64_t value;
            };
            using TimelineSemaphoreSignalData = std::tuple<TimelineSemaphore, SignalValue, VkPipelineStageFlagBits2>;
        public:

            template<uint32_t OtherFlag>
            friend class GpuLinkBuilder;

            explicit GpuLinkBuilder(RenderGraph::Link& link, Graph& graph)
                : _link(link)
                , _graph(graph)
            {}

            template<uint32_t OtherFlag>
            GpuLinkBuilder(GpuLinkBuilder<OtherFlag>&& o)
                : _link(o._link)
                , _graph(o._graph)
                , _signal_data(std::move(o._signal_data))
            {

            }

        private:

            Link& _link;
            Graph& _graph;

            std::variant<EmptySignalData, BinarySemaphoreSignalData, TimelineSemaphoreSignalData> _signal_data;
        };



        template<>
        class RenderGraphBuilder::GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_WAIT_EXTERNAL>
        {
        private:
            using EmptySignalData = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::EmptySignalData;
            using BinarySemaphoreSignalData = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::BinarySemaphoreSignalData;
            using TimelineSemaphoreSignalData = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::TimelineSemaphoreSignalData;
            using SignalValue = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::SignalValue;
        public:
            template<uint32_t OtherFlag>
            friend class GpuLinkBuilder;
            explicit GpuLinkBuilder(RenderGraph::Link& link, Graph& graph)
                : _link(link)
                , _graph(graph)
            {}

            template<uint32_t OtherFlag>
            GpuLinkBuilder(GpuLinkBuilder<OtherFlag>&& o)
                : _link(o._link)
                , _graph(o._graph)
                , _signal_data(std::move(o._signal_data))
            {

            }

            GpuLinkBuilder<OP_FLAG_NOP> waitOnGpu(BinarySemaphore semaphore, VkPipelineStageFlagBits2 wait_mask)&&
            {
                using ExternalConnection = RenderGraph::Link::ExternalConnection;
                using PipelineConnection = RenderGraph::Link::PipelineConnection;
                std::visit(overloaded(
                    [&](const EmptySignalData&) { _link.addConnection(ExternalConnection(semaphore.getName(), wait_mask, std::nullopt)); },
                    [&](const BinarySemaphoreSignalData&) { assert(false && "Operation is not allowed"); },
                    [&](const TimelineSemaphoreSignalData&) { assert(false && "Operation is not allowed"); }
                ), _signal_data);
                return { std::move(*this) };
            }
            GpuLinkBuilder<OP_FLAG_NOP> waitOnGpu(TimelineSemaphore semaphore, int64_t value, VkPipelineStageFlagBits2 wait_mask)&&
            {
                using ExternalConnection = RenderGraph::Link::ExternalConnection;
                using PipelineConnection = RenderGraph::Link::PipelineConnection;
                std::visit(overloaded(
                    [&](const EmptySignalData&) { _link.addConnection(ExternalConnection(semaphore.getName(), wait_mask, value)); },
                    [&](const BinarySemaphoreSignalData&) { assert(false && "Operation is not allowed"); },
                    [&](const TimelineSemaphoreSignalData&) { assert(false && "Operation is not allowed"); }
                ), _signal_data);
                return { std::move(*this) };
            }
        private:

            Link& _link;
            Graph& _graph;

            std::variant<EmptySignalData, BinarySemaphoreSignalData, TimelineSemaphoreSignalData> _signal_data;
        };

        template<>
        class RenderGraphBuilder::GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_WAIT>
        {
        private:
            using EmptySignalData = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::EmptySignalData;
            using BinarySemaphoreSignalData = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::BinarySemaphoreSignalData;
            using SignalValue = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::SignalValue;
            using TimelineSemaphoreSignalData = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::TimelineSemaphoreSignalData;
        public:
            template<uint32_t OtherFlag>
            friend class GpuLinkBuilder;
            explicit GpuLinkBuilder(RenderGraph::Link& link, Graph& graph)
                : _link(link)
                , _graph(graph)
            {}

            template<uint32_t OtherFlag>
            GpuLinkBuilder(GpuLinkBuilder<OtherFlag>&& o)
                : _link(o._link)
                , _graph(o._graph)
                , _signal_data(std::move(o._signal_data))
            {

            }

            GpuLinkBuilder<OP_FLAG_NOP> waitOnGpu(VkPipelineStageFlagBits2 wait_mask)&&
            {
                using ExternalConnection = RenderGraph::Link::ExternalConnection;
                using PipelineConnection = RenderGraph::Link::PipelineConnection;
                std::visit(overloaded(
                    [&](const EmptySignalData&) { assert(false && "Operation is not allowed"); },
                    [&](const BinarySemaphoreSignalData& data) { _link.addConnection(PipelineConnection(std::get<BinarySemaphore>(data).getName(), std::get<VkPipelineStageFlagBits2>(data), wait_mask, std::nullopt)); },
                    [&](const TimelineSemaphoreSignalData& data) { _link.addConnection(PipelineConnection(std::get<TimelineSemaphore>(data).getName(), std::get<VkPipelineStageFlagBits2>(data), wait_mask, std::get<SignalValue>(data).value)); }
                ), _signal_data);
                return { std::move(*this) };
            }

        private:

            Link& _link;
            Graph& _graph;

            std::variant<EmptySignalData, BinarySemaphoreSignalData, TimelineSemaphoreSignalData> _signal_data;
        };

        template<>
        class RenderGraphBuilder::GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_SIGNAL>
        {
        private:
            using EmptySignalData = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::EmptySignalData;
            using BinarySemaphoreSignalData = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::BinarySemaphoreSignalData;
            using SignalValue = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::SignalValue;
            using TimelineSemaphoreSignalData = GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::TimelineSemaphoreSignalData;
        public:
            template<uint32_t OtherFlag>
            friend class GpuLinkBuilder;
            explicit GpuLinkBuilder(RenderGraph::Link& link, Graph& graph)
                : _link(link)
                , _graph(graph)
            {}

            template<uint32_t OtherFlag>
            GpuLinkBuilder(GpuLinkBuilder<OtherFlag>&& o)
                : _link(o._link)
                , _graph(o._graph)
                , _signal_data(std::move(o._signal_data))
            {

            }
            GpuLinkBuilder<OP_FLAG_WAIT_EXTERNAL> signalOnGpu()&&
            {
                return { std::move(*this) };
            }
            GpuLinkBuilder<OP_FLAG_WAIT> signalOnGpu(VkPipelineStageFlagBits2 signal_mask)&&
            {
                auto semaphore = BinarySemaphore{ generateSemaphoreName() };
                _graph.registerSemaphore(semaphore);
                _signal_data = std::make_tuple(semaphore, signal_mask);
                return { std::move(*this) };
            }
            GpuLinkBuilder<OP_FLAG_WAIT> signalOnGpu(const TimelineSemaphore& semaphore, uint64_t value, VkPipelineStageFlagBits2 signal_mask)&&
            {
                _signal_data = std::make_tuple(semaphore, GpuLinkBuilder<RenderGraphBuilder::OP_FLAG_NOP>::SignalValue{ value }, signal_mask);
                return { std::move(*this) };

            }
            GpuLinkBuilder<OP_FLAG_WAIT> signalOnGpu(const BinarySemaphore& semaphore, VkPipelineStageFlagBits2 signal_mask)&&
            {
                _signal_data = std::make_tuple(semaphore, signal_mask);
                return { std::move(*this) };
            }

        private:

            Link& _link;
            Graph& _graph;

            std::variant<EmptySignalData, BinarySemaphoreSignalData, TimelineSemaphoreSignalData> _signal_data;

            std::string generateSemaphoreName()
            {
                // TODO: add unique name generation
                return std::format("{:s}-{:s}", _link.getFromNode()->getName(), _link.getToNode()->getName());
            }
        };



    }
}