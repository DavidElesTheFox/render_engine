#pragma once

#include <render_engine/rendergraph/Link.h>
#include <render_engine/rendergraph/Node.h>


#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace RenderEngine::RenderGraph
{
    class GraphVisitor;

    class BinarySemaphore
    {
    public:
        explicit BinarySemaphore(std::string name)
            : _name(std::move(name))
        {}
        const std::string& getName() const { return _name; }
    private:
        std::string _name;
    };

    class TimelineSemaphore
    {
    public:
        TimelineSemaphore(std::string name, uint64_t initial_value, uint64_t value_range)
            : _name(std::move(name))
            , _initial_value(initial_value)
            , _value_range(value_range)
        {}
        const std::string& getName() const { return _name; }
        uint64_t getInitialValue() const { return _initial_value; }
        uint64_t getValueRange() const { return _value_range; }

    private:
        std::string _name;
        uint64_t _initial_value{ 0 };
        uint64_t _value_range{ 0 };
    };

    class Graph
    {
    public:

        explicit Graph(std::string name)
            : _name(std::move(name))
        {}
        ~Graph() = default;

        Graph(Graph&&) = default;
        Graph(const Graph&) = delete;

        Graph& operator=(Graph&&) = default;
        Graph& operator=(const Graph&) = delete;

        Node& addNode(std::unique_ptr<Node> node);
        Link& addEdge(std::unique_ptr<Link> edge);
        Node& replaceNode(const Node* old_node, std::unique_ptr<Node> new_node);

        void removeNode(const Node* node);
        void removeEdge(const Node* from, const Node* to);

        void accept(GraphVisitor& visitor);

        std::vector<const Node*> findPredecessors(const Node* node, LinkType accepted_links = LinkType::Unknown) const;
        std::vector<const Node*> findAllPredecessors(const Node* node, LinkType accepted_links = LinkType::Unknown) const;

        std::vector<const Node*> findSuccessor(const Node* node, LinkType accepted_links = LinkType::Unknown) const;
        std::vector<const Node*> findAllSuccessor(const Node* node, LinkType accepted_links = LinkType::Unknown) const;

        const Node* findNode(const std::string& name) const;
        std::vector<const Link*> findEdgesTo(const Node* node) const;
        std::vector<const Link*> findEdgesFrom(const Node* node) const;

        void applyChanges();

        void registerSemaphore(BinarySemaphore semaphore);
        void registerSemaphore(TimelineSemaphore semaphore);

        std::ranges::input_range auto getSemaphoreDefinitions() const { return _semaphore_definitions | std::views::values; }

    private:
        struct GraphRepresentation
        {
            std::unordered_map<std::string, std::unique_ptr<Node>> nodes;
            std::unordered_map<const Node*, std::vector<std::unique_ptr<Link>>> in_edges;
            std::unordered_map<const Node*, std::vector<Link*>> out_edges;

            void addNode(std::unique_ptr<Node> node);
            void addEdge(std::unique_ptr<Link> link);
            void replaceNode(const Node* old_node, std::unique_ptr<Node> new_node);

            void removeNode(const Node* node);
            void removeEdge(const Link* from);
            std::vector<const Node*> findPredecessors(const Node* node, LinkType accepted_links) const;
            std::vector<const Node*> findSuccessor(const Node* node, LinkType accepted_links) const;
            std::vector<const Link*> findEdgesTo(const Node* node) const;
            std::vector<const Link*> findEdgesFrom(const Node* node) const;
            const Node* findNode(const std::string& name) const;
            const Link* findEdge(const Node* from, const Node* to) const;
        };

        template<typename T>
        class CommandWrapper
        {
        public:
            CommandWrapper(std::function<void(GraphRepresentation&, std::unique_ptr<T>&&, const T*)>&& command, std::unique_ptr<T> object)
                : _command(std::move(command))
                , _owned_object(std::move(object))
            {}
            CommandWrapper(std::function<void(GraphRepresentation&, std::unique_ptr<T>&&, const T*)>&& command, const T* object)
                : _command(std::move(command))
                , _object(object)
            {}
            CommandWrapper(std::function<void(GraphRepresentation&, std::unique_ptr<T>&&, const T*)>&& command, std::unique_ptr<T> owned_object, const T* object)
                : _command(std::move(command))
                , _owned_object(std::move(owned_object))
                , _object(object)
            {}
            CommandWrapper(CommandWrapper&&) = default;
            CommandWrapper(const CommandWrapper&) = delete;

            CommandWrapper& operator=(CommandWrapper&&) = default;
            CommandWrapper& operator=(const CommandWrapper&) = delete;

            void operator()(GraphRepresentation& graph)
            {
                _command(graph, std::move(_owned_object), _object);
            }
        private:
            std::function<void(GraphRepresentation&, std::unique_ptr<T>&&, const T*)> _command;
            std::unique_ptr<T> _owned_object;
            const T* _object{ nullptr };
        };

        std::string _name;
        GraphRepresentation _graph;

        std::unordered_map<std::string, std::variant<TimelineSemaphore, BinarySemaphore>> _semaphore_definitions;

        std::vector<CommandWrapper<Node>> _add_node_commands;
        std::vector<CommandWrapper<Link>> _add_edge_commands;
        std::vector<CommandWrapper<Link>> _remove_edge_commands;
        std::vector<CommandWrapper<Node>> _remove_node_commands;
        std::vector<CommandWrapper<Node>> _replace_node_commands;

        mutable std::mutex _staging_area_mutex;
        mutable std::shared_mutex _graph_mutex;
    };
}