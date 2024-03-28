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
                                   Job::ExecutionContext& execution_context,
                                   LogicalDevice& logical_device,
                                   uint32_t backbuffer_count)
                : GraphVisitor(graph)
                , _logical_device(logical_device)
                , _backbuffer_size(backbuffer_count)
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
            }
        private:

            struct TaskDetails
            {
                TaskDetails(tf::Task task, std::unique_ptr<Job> job)
                    : task(task), job(std::move(job))
                {}
                tf::Task task;
                std::unique_ptr<Job> job;
            };

            void visitImpl(Node* node);
            void collectSyncOperationForNode(const Node* node, SyncObject& sync_object) const
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
                        link->addSyncOperationsForDestination(sync_object, _backbuffer_size);
                    }
                }
                for (auto* link : getGraph().findEdgesFrom(node))
                {
                    link->addSyncOperationsForSource(sync_object, _backbuffer_size);
                }
            }

            LogicalDevice& _logical_device;
            uint32_t _backbuffer_size{ 0 };
            Job::ExecutionContext& _execution_context;
            tf::Taskflow _task_container;
            std::unordered_map<std::string, TaskDetails> _task_map;
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
                    task_before.task.precede(task_later.task);
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
                    task_after.task.succeed(task_before.task);
                }
            }
        }

        void VisitorForTaskCreation::visitImpl(Node* node)
        {
            if (node->isActive() == false)
            {
                return;
            }
            SyncObject sync_object{ _logical_device };
            collectSyncOperationForNode(node, sync_object);
            auto job = node->createJob(sync_object.getOperationsGroups());
            auto tf_task = _task_container.emplace([job_to_execute = job.get(),
                                                   stored_sync_object = std::move(sync_object),
                                                   &execution_context_for_job = _execution_context]
                                                   {
                                                       job_to_execute->execute(execution_context_for_job);
                                                   });
            _task_map.insert({ node->getName(), TaskDetails{ std::move(tf_task), std::move(job) } });
        }
    }


    tf::Taskflow TaskflowBuilder::createTaskflow(Graph& graph,
                                                 Job::ExecutionContext& execution_context,
                                                 LogicalDevice& logical_device,
                                                 uint32_t backbuffer_count)
    {
        VisitorForTaskCreation visitor(graph, execution_context, logical_device, backbuffer_count);
        visitor.run();
        return visitor.clear();
    }

}