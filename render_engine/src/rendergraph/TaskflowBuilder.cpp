#include <render_engine/rendergraph/TaskflowBuilder.h>

#include <render_engine/rendergraph/GraphVisitor.h>

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
                                   const std::vector<SyncObject*>& sync_objects)
                : GraphVisitor(graph)
                , _logical_device(logical_device)
                , _sync_objects(sync_objects)
                , _execution_context(execution_context)

            {}

            void visit(RenderNode* node) override { visitImpl(node); }
            void visit(TransferNode* node) override { visitImpl(node); };
            void visit(ComputeNode* node) override { visitImpl(node); };
            void visit(PresentNode* node) override { visitImpl(node); };
            void visit(CpuNode* node) override { visitImpl(node); };
            void visit(Link* edge) override;

            tf::Taskflow clear()
            {
                _task_map.clear();
                return std::move(_task_container);
            };

        private:

            void visitImpl(Node* node);
            void collectSyncOperationForNode(const Node* node, SyncObject& sync_object)
            {

                if (node == nullptr)
                {
                    return;
                }
                for (auto* link : getGraph().findEdgesTo(node))
                {
                    if (link->getFromNode() == nullptr)
                    {
                        continue;
                    }
                    if (link->getFromNode()->isActive() == false)
                    {
                        collectSyncOperationForNode(link->getFromNode(), sync_object);
                    }
                    else
                    {
                        link->forEachConnections([&](const Link::PipelineConnection& connection) { addSynOperationsForDestination(connection, node->getName()); },
                                                 [&](const Link::ExternalConnection& connection) { addSynOperationsForDestination(connection, node->getName()); });
                    }
                }
                for (auto* link : getGraph().findEdgesFrom(node))
                {
                    link->forEachConnections([&](const Link::PipelineConnection& connection) { addSynOperationsForSource(connection, node->getName()); },
                                             [&](const Link::ExternalConnection& connection) { addSynOperationsForSource(connection, node->getName()); });
                }
            }

            void addSynOperationsForSource(const Link::PipelineConnection& connection, const std::string& node_name)
            {
                for (auto& sync_object : _sync_objects)
                {
                    if (connection.value == std::nullopt)
                    {
                        sync_object->addSignalOperationToGroup(node_name,
                                                               connection.semaphore_name,
                                                               connection.signal_stage);
                    }
                    else
                    {
                        sync_object->addSignalOperationToGroup(node_name,
                                                               connection.semaphore_name,
                                                               connection.signal_stage,
                                                               *connection.value);
                    }
                }
            }
            void addSynOperationsForSource(const Link::ExternalConnection&, const std::string&)
            {}

            void addSynOperationsForDestination(const Link::PipelineConnection& connection, const std::string& node_name)
            {
                for (auto& sync_object : _sync_objects)
                {
                    if (connection.value == std::nullopt)
                    {
                        sync_object->addWaitOperationToGroup(node_name,
                                                             connection.semaphore_name,
                                                             connection.signal_stage);
                    }
                    else
                    {
                        sync_object->addWaitOperationToGroup(node_name,
                                                             connection.semaphore_name,
                                                             connection.signal_stage,
                                                             *connection.value);
                    }
                }
            }
            void addSynOperationsForDestination(const Link::ExternalConnection& connection, const std::string& node_name)
            {
                for (auto& sync_object : _sync_objects)
                {
                    sync_object->addWaitOperationToGroup(node_name,
                                                         connection.signaled_semaphore_name,
                                                         connection.wait_stage);
                }
            }

            LogicalDevice& _logical_device;
            const std::vector<SyncObject*>& _sync_objects;
            ExecutionContext& _execution_context;
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
            if (node->isActive() == false)
            {
                return;
            }
            SyncObject sync_object{ _logical_device, std::format("Node-{:s}", node->getName()) }; // this sync object cannot die during the process.
            collectSyncOperationForNode(node, sync_object);

            node->register_execution_context(_execution_context);

            auto tf_task = _task_container.emplace([node, &execution_context_for_job = _execution_context]
                                                   {
                                                       // TODO implement tracking: Probably the parameter can go to a member of the node
                                                       node->execute(execution_context_for_job, nullptr);
                                                   });
            assert(_task_map.contains(node->getName()) == false);
            _task_map.insert({ node->getName(), std::move(tf_task) });
        }
    }


    tf::Taskflow TaskflowBuilder::createTaskflow(Graph& graph,
                                                 ExecutionContext& execution_context,
                                                 LogicalDevice& logical_device,
                                                 const std::vector<SyncObject*>& sync_objects)
    {
        VisitorForTaskCreation visitor(graph, execution_context, logical_device, sync_objects);
        visitor.run();
        return visitor.clear();
    }

}