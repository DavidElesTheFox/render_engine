#include <render_engine/rendergraph/Graph.h>

#include <algorithm>
#include <deque>
#include <ranges>

namespace RenderEngine::RenderGraph
{

    namespace
    {
        std::string getNodeName(const Node* node)
        {
            if (node == nullptr)
            {
                return "END";
            }
            return node->getName();
        }

        std::string edgeToString(const Link& edge)
        {
            return getNodeName(edge.getFromNode()) + " -> " + getNodeName(edge.getToNode());
        }

    }
    void Graph::GraphRepresentation::addNode(std::unique_ptr<Node> node)
    {
        nodes.insert({ node->getName(), std::move(node) });
    }

    void Graph::GraphRepresentation::addEdge(std::unique_ptr<Link> link)
    {
        auto& edge_list = in_edges[link->getToNode()];
        auto it = std::ranges::find_if(edge_list, [&](const auto& edge) { return edge->getFromNode() == link->getFromNode(); });
        if (it != edge_list.end())
        {
            throw std::runtime_error("Edge is already defined between the two nodes: "
                                     + edgeToString(**it));
        }
        auto from_node = link->getFromNode();
        edge_list.push_back(std::move(link));
        out_edges[from_node].push_back(edge_list.back().get());

    }

    void Graph::GraphRepresentation::removeNode(const Node* node)
    {
        assert(node != nullptr);
        for (auto& edge_list : in_edges | std::views::values)
        {
            for (auto& edge_ptr : edge_list)
            {
                if (node == edge_ptr->getFromNode()
                    || node == edge_ptr->getToNode())
                {
                    throw std::runtime_error("Node cannot be deleted it has still edge in the graph: "
                                             + edgeToString(*edge_ptr));
                }
            }
        }

        nodes.erase(node->getName());
    }

    void Graph::GraphRepresentation::removeEdge(const Link* edge)
    {
        if (edge == nullptr)
        {
            return;
        }
        const Node* from = edge->getFromNode();
        const Node* to = edge->getToNode();
        // Remove all the in edges where the from is the from
        [[maybe_unused]] size_t deleted_in_edges = std::erase_if(in_edges.at(to), [=](const auto& edge) { return edge->getFromNode() == from; });
        // Remove all the out edges where the to is the to
        [[maybe_unused]] size_t deleted_out_edges = std::erase_if(out_edges.at(from), [=](const auto& edge) { return edge->getToNode() == to; });
        assert(deleted_in_edges == deleted_out_edges);

    }

    std::vector<const Node*> Graph::GraphRepresentation::findPredecessors(const Node* node, LinkType accepted_links) const
    {
        auto it = in_edges.find(node);
        if (it == in_edges.end())
        {
            return {};
        }
        std::vector<const Node*> result;
        for (const auto& edge_ptr : it->second)
        {
            if (accepted_links == LinkType::Unknown
                || edge_ptr->getType() == accepted_links)
            {
                result.push_back(edge_ptr->getFromNode());
            }
        }
        return result;
    }

    std::vector<const Node*> Graph::GraphRepresentation::findSuccessor(const Node* node, LinkType accepted_links) const
    {
        auto it = out_edges.find(node);
        if (it == out_edges.end())
        {
            return {};
        }
        std::vector<const Node*> result;
        for (const auto& edge_ptr : it->second)
        {
            if (accepted_links == LinkType::Unknown
                || edge_ptr->getType() == accepted_links)
            {
                result.push_back(edge_ptr->getToNode());
            }
        }
        return result;
    }

    std::vector<const Link*> Graph::GraphRepresentation::findEdgesTo(const Node* node) const
    {
        auto it = in_edges.find(node);
        if (it == in_edges.end())
        {
            return {};
        }
        std::vector<const Link*> result;
        result.reserve(it->second.size());
        std::transform(it->second.begin(), it->second.end(),
                       std::back_inserter(result),
                       [](auto& ptr) { return ptr.get(); });
        return result;
    }

    std::vector<const Link*> Graph::GraphRepresentation::findEdgesFrom(const Node* node) const
    {
        auto it = out_edges.find(node);
        if (it == out_edges.end())
        {
            return {};
        }
        std::vector<const Link*> result(it->second.begin(), it->second.end());
        return result;
    }

    const Node* Graph::GraphRepresentation::findNode(const std::string& name) const
    {
        auto it = nodes.find(name);
        return it == nodes.end() ? nullptr : it->second.get();
    }

    const Link* Graph::GraphRepresentation::findEdge(const Node* from, const Node* to) const
    {
        auto it = in_edges.find(to);
        if (it == in_edges.end())
        {
            return nullptr;
        }
        auto found_link_it = std::ranges::find_if(it->second, [=](auto& edge) { return from == edge->getFromNode(); });
        return found_link_it == it->second.end() ? nullptr : found_link_it->get();
    }

    Node& Graph::addNode(std::unique_ptr<Node> node)
    {
        Node& result = *node;

        std::lock_guard lock{ _staging_area_mutex };
        auto callback = [](GraphRepresentation& graph, std::unique_ptr<Node>&& node_to_add, const Node*)
            {
                graph.addNode(std::move(node_to_add));
            };
        _add_node_commands.emplace_back(CommandWrapper<Node>{ std::move(callback), std::move(node) });
        return result;
    }

    Link& Graph::addEdge(std::unique_ptr<Link> edge)
    {
        Link& result = *edge;
        std::lock_guard lock{ _staging_area_mutex };
        auto callback = [](GraphRepresentation& graph, std::unique_ptr<Link>&& link_to_add, const Link*)
            {
                graph.addEdge(std::move(link_to_add));
            };

        _add_edge_commands.emplace_back(CommandWrapper<Link>{ std::move(callback), std::move(edge) });
        return result;
    }

    void Graph::removeNode(const Node* node)
    {
        std::lock_guard lock{ _staging_area_mutex };
        auto callback = [](GraphRepresentation& graph, std::unique_ptr<Node>&&, const Node* node_to_remove)
            {
                graph.removeNode(node_to_remove);
            };
        _remove_node_commands.emplace_back(CommandWrapper<Node>(std::move(callback), node));
    }

    void Graph::removeEdge(const Node* from, const Node* to)
    {
        std::lock_guard lock{ _staging_area_mutex };
        const Link* link = _graph.findEdge(from, to);
        auto callback = [](GraphRepresentation& graph, std::unique_ptr<Link>&&, const Link* link_to_remove)
            {
                graph.removeEdge(link_to_remove);
            };
        _remove_edge_commands.emplace_back(CommandWrapper<Link>(std::move(callback), link));
    }

    void Graph::accept(GraphVisitor& visitor)
    {
        std::shared_lock lock{ _graph_mutex };
        for (auto& node_ptr : _graph.nodes | std::views::values)
        {
            node_ptr->accept(visitor);
        }
        for (auto& edge_list : _graph.in_edges | std::views::values)
        {
            for (auto& edge_ptr : edge_list)
            {
                edge_ptr->accept(visitor);
            }
        }
    }


    std::vector<const Node*> Graph::findPredecessors(const Node* node, LinkType accepted_links) const
    {
        std::shared_lock lock{ _graph_mutex };

        return _graph.findPredecessors(node, accepted_links);
    }

    std::vector<const Node*> Graph::findAllPredecessors(const Node* node, LinkType accepted_links) const
    {
        std::shared_lock lock{ _graph_mutex };

        std::vector<const Node*> result;
        std::deque<const Node*> open_set = { node };
        while (open_set.empty() == false)
        {
            const Node* current_node = open_set.front();
            open_set.pop_front();

            result.push_back(current_node);

            auto predecessors = _graph.findPredecessors(current_node, accepted_links);
            for (const Node* parent : predecessors)
            {
                if (parent == nullptr)
                {
                    continue;
                }
                open_set.push_back(parent);
            }
        }
        return result;
    }

    std::vector<const Node*> Graph::findSuccessor(const Node* node, LinkType accepted_links) const
    {
        std::shared_lock lock{ _graph_mutex };

        return _graph.findSuccessor(node, accepted_links);
    }

    std::vector<const Node*> Graph::findAllSuccessor(const Node* node, LinkType accepted_links) const
    {
        std::shared_lock lock{ _graph_mutex };

        std::vector<const Node*> result;
        std::deque<const Node*> open_set = { node };
        while (open_set.empty() == false)
        {
            const Node* current_node = open_set.front();
            open_set.pop_front();

            result.push_back(current_node);

            auto predecessors = _graph.findSuccessor(current_node, accepted_links);
            for (const Node* parent : predecessors)
            {
                if (parent == nullptr)
                {
                    continue;
                }
                open_set.push_back(parent);
            }
        }
        return result;
    }

    const Node* Graph::findNode(const std::string& name) const
    {
        std::shared_lock lock{ _graph_mutex };
        return _graph.findNode(name);
    }

    std::vector<const Link*> Graph::findEdgesTo(const Node* node) const
    {
        std::shared_lock lock{ _graph_mutex };
        return _graph.findEdgesTo(node);
    }
    std::vector<const Link*> Graph::findEdgesFrom(const Node* node) const
    {
        std::shared_lock lock{ _graph_mutex };
        return _graph.findEdgesFrom(node);
    }

    void Graph::applyChanges()
    {
        std::scoped_lock lock{ _staging_area_mutex, _graph_mutex };
        for (auto& callback : _add_node_commands)
        {
            callback(_graph);
        }
        for (auto& callback : _add_edge_commands)
        {
            callback(_graph);
        }
        for (auto& callback : _remove_edge_commands)
        {
            callback(_graph);
        }
        for (auto& callback : _remove_node_commands)
        {
            callback(_graph);
        }
        _add_node_commands.clear();
        _add_edge_commands.clear();
        _remove_node_commands.clear();
        _remove_edge_commands.clear();
    }

    void Graph::registerSemaphore(BinarySemaphore semaphore)
    {
        if (_semaphore_definitions.contains(semaphore.getName()))
        {
            throw std::runtime_error("Semaphore already defined");
        }
        _semaphore_definitions.insert({ semaphore.getName(), std::move(semaphore) });
    }

    void Graph::registerSemaphore(TimelineSemaphore semaphore)
    {
        if (_semaphore_definitions.contains(semaphore.getName()))
        {
            throw std::runtime_error("Semaphore already defined");
        }
        _semaphore_definitions.insert({ semaphore.getName(), std::move(semaphore) });
    }

}