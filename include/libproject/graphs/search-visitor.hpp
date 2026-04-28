#pragma once

namespace libproject::graphs
{
    enum class ControlAction
    {
        Continue,
        Skip,
        Stop
    };

    template <typename V>
    concept SearchVisitor = requires(V & visitor, VertexID u, VertexID v)
    {
        { visitor.source_vertex(u) };
        { visitor.discover_vertex(u) };
        { visitor.expand_vertex(u) };
        { visitor.examine_edge(u, v) };
        { visitor.tree_edge(u, v) };
        { visitor.non_tree_edge(u, v) };
        { visitor.finish_vertex(u) };
    };

    template <typename V>
    concept DiscoverControllable= requires (V & visitor,
                                               VertexID u)
    {
        { visitor.discover_vertex(u) } -> std::same_as<ControlAction>;
    };

    template <typename V>
    concept ExamineControllable = requires (V & visitor,
                                               VertexID u,
                                               VertexID v)
    {
        { visitor.examine_edge(u, v) } -> std::same_as<ControlAction>;
    };

    struct NoOpSearchVisitor
    {
        void source_vertex(VertexID u)
        {}

        void discover_vertex(VertexID u)
        {}

        void expand_vertex(VertexID u)
        {}

        void examine_edge(VertexID u, VertexID v)
        {}

        void tree_edge(VertexID u, VertexID v)
        {}

        void non_tree_edge(VertexID u, VertexID v)
        {}

        void finish_vertex(VertexID u)
        {}
    };
};
