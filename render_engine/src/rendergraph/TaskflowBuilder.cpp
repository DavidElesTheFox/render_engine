#include <render_engine/rendergraph/TaskflowBuilder.h>

#include <render_engine/Debugger.h>
#include <render_engine/RenderContext.h>

#include <render_engine/rendergraph/GraphVisitor.h>
#include <render_engine/rendergraph/Topic.h>

namespace RenderEngine::RenderGraph
{
    namespace
    {
        class VisitorForTaskCreation : public GraphVisitor
        {
        public:
            VisitorForTaskCreation(Graph& graph,
                                   ExecutionContext& execution_context,
                                   LogicalDevice& logical_device,
                                   SyncObject* sync_object,
                                   SyncFeedbackService& feedback_service)
                : GraphVisitor(graph)
                , _logical_device(logical_device)
                , _sync_object(sync_object)
                , _execution_context(execution_context)
                , _feedback_service(feedback_service)

            {}

            void visit(RenderNode* node) override { visitImpl(node); }
            void visit(TransferNode* node) override { visitImpl(node); };
            void visit(ComputeNode* node) override { visitImpl(node); };
            void visit(PresentNode* node) override { visitImpl(node); };
            void visit(DeviceSynchronizeNode* node) override { visitImpl(node); };
            void visit(CpuNode* node) override { visitImpl(node); };
            void visit(Link* edge) override;

            tf::Taskflow clear()
            {
                _task_map.clear();
                return std::move(_task_container);
            };

        private:
            void preRun() override
            {
                RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "==================================================");
                RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "|                 BUILDING GRAPH                 |");
                RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "==================================================");
            }
            void postRun() override
            {
                RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "==================================================");
                RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "|               BUILDING GRAPH END               |");
                RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "==================================================");
            }

            void visitImpl(Node* node);
            void collectSyncOperationForNode(const Node* node)
            {
                if (node == nullptr)
                {
                    return;
                }
                for (auto* link : getGraph().findEdgesTo(node))
                {
                    RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "   link:{:s}->{:s}", link->getFromNode()->getName(), link->getToNode()->getName());

                    if (link->getFromNode() == nullptr)
                    {
                        continue;
                    }
                    link->forEachConnections([&](const Link::PipelineConnection& connection) { addSynOperationsForDestination(connection, node->getName()); },
                                             [&](const Link::ExternalConnection& connection) { addSynOperationsForDestination(connection, node->getName()); });
                }
                for (auto* link : getGraph().findEdgesFrom(node))
                {
                    link->forEachConnections([&](const Link::PipelineConnection& connection) { addSynOperationsForSource(connection, node->getName()); },
                                             [&](const Link::ExternalConnection& connection) { addSynOperationsForSource(connection, node->getName()); });
                }
            }

            void addSynOperationsForSource(const Link::PipelineConnection& connection, const std::string& node_name)
            {
                RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "      Add signal operation for node: {:s} ({:s}) into: {:s}",
                                                             node_name,
                                                             connection.semaphore_name,
                                                             _sync_object->getName());
                if (connection.value == std::nullopt)
                {
                    _sync_object->addSignalOperationToGroup(node_name,
                                                            connection.semaphore_name,
                                                            connection.signal_stage);
                }
                else
                {
                    _sync_object->addSignalOperationToGroup(node_name,
                                                            connection.semaphore_name,
                                                            connection.signal_stage,
                                                            *connection.value);
                }
            }
            void addSynOperationsForSource(const Link::ExternalConnection&, const std::string&)
            {}

            void addSynOperationsForDestination(const Link::PipelineConnection& connection, const std::string& node_name)
            {
                RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "      Add wait operation for node: {:s} ({:s}) into: {:s}",
                                                             node_name,
                                                             connection.semaphore_name,
                                                             _sync_object->getName());
                if (connection.value == std::nullopt)
                {
                    _sync_object->addWaitOperationToGroup(node_name,
                                                          connection.semaphore_name,
                                                          connection.signal_stage);
                }
                else
                {
                    _sync_object->addWaitOperationToGroup(node_name,
                                                          connection.semaphore_name,
                                                          connection.signal_stage,
                                                          *connection.value);
                }
            }
            void addSynOperationsForDestination(const Link::ExternalConnection& connection, const std::string& node_name)
            {

                RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "      Add external signal operation for node: {:s} ({:s}) into: {:s}",
                                                             node_name,
                                                             connection.signaled_semaphore_name,
                                                             _sync_object->getName());
                _sync_object->addWaitOperationToGroup(node_name,
                                                      connection.signaled_semaphore_name,
                                                      connection.wait_stage);
            }

            LogicalDevice& _logical_device;
            SyncObject* _sync_object{ nullptr };
            ExecutionContext& _execution_context;
            SyncFeedbackService& _feedback_service;
            tf::Taskflow _task_container;
            std::unordered_map<std::string, tf::Task> _task_map;
        };
        void VisitorForTaskCreation::visit(Link* edge)
        {
            if (edge->getType() != LinkType::CpuSync)
            {
                return;
            }
            assert(edge->getToNode() != nullptr);
            assert(edge->getFromNode() != nullptr);

            // Link all the task dependences before this node that can run parallel on CPU
            {
                auto& task_later = _task_map.at(edge->getToNode()->getName());

                std::vector<const Node*> all_node_before = getGraph().findAllPredecessors(edge->getFromNode(), LinkType::CpuSync);
                all_node_before.push_back(edge->getFromNode());

                for (auto& node_before : all_node_before)
                {
                    auto& task_before = _task_map.at(node_before->getName());
                    task_before.precede(task_later);
                }
            }
            // Link all the task dependences after this node that can run parallel on CPU
            {
                auto& task_before = _task_map.at(edge->getFromNode()->getName());

                std::vector<const Node*> all_node_after = getGraph().findAllSuccessor(edge->getToNode(), LinkType::CpuSync);
                all_node_after.push_back(edge->getToNode());

                for (auto& node_after : all_node_after)
                {
                    auto& task_after = _task_map.at(node_after->getName());
                    task_after.succeed(task_before);
                }
            }
        }

        void VisitorForTaskCreation::visitImpl(Node* node)
        {
            RenderContext::context().getDebugger().print(Debug::Topics::RenderGraphBuilder{}, "Visit node: " + node->getName());
            collectSyncOperationForNode(node);

            node->registerExecutionContext(_execution_context);
            QueueSubmitTracker* submit_tracker = nullptr;
            {
                auto tracker = std::make_unique<QueueSubmitTracker>(_logical_device, node->getName());
                submit_tracker = tracker.get();
                _feedback_service.registerTracker(_sync_object, node->getName(), std::move(tracker));
            }

            auto tf_task = _task_container.emplace([node, &execution_context_for_job = _execution_context, submit_tracker]
                                                   {
                                                       if (node->isActive() == false)
                                                       {
                                                           return;
                                                       }
                                                       node->execute(execution_context_for_job, submit_tracker);
                                                   });
            assert(_task_map.contains(node->getName()) == false);
            _task_map.insert({ node->getName(), std::move(tf_task) });
        }
    }


    tf::Taskflow TaskflowBuilder::createTaskflow(Graph& graph,
                                                 ExecutionContext& execution_context,
                                                 LogicalDevice& logical_device,
                                                 SyncObject* sync_object,
                                                 SyncFeedbackService& feedback_service)
    {
        VisitorForTaskCreation visitor(graph, execution_context, logical_device, sync_object, feedback_service);
        visitor.run();
        return visitor.clear();
    }

}