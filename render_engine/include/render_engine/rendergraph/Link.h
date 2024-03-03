#pragma once

#include <render_engine/synchronization/SyncObject.h>

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
        Link(const Node* from, const Node* to, LinkType type, SyncObject&& sync_object)
            : _from_node(from)
            , _to_node(to)
            , _type(type)
            , _syncObject(std::move(sync_object))
        {}

        Link(Link&&) = delete;
        Link(const Link&) = delete;

        Link& operator=(Link&&) = delete;
        Link& operator=(const Link&) = delete;

        void accept(GraphVisitor&);

        LinkType getType() const { return _type; }
        const Node* getFromNode() const { return _from_node; }
        const Node* getToNode() const { return _to_node; }
        const SyncObject& getSyncObject() const { return _syncObject; }
    private:
        LinkType _type{ LinkType::Unknown };
        const Node* _from_node{ nullptr };
        const Node* _to_node{ nullptr };
        SyncObject _syncObject;
    };
}