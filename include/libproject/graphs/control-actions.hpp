#pragma once

#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{
    enum class ControlAction
    {
        Continue,
        Skip,
        Stop
    };

    // For BFS/DFS
    template <typename V>
    concept DiscoverControllable= requires (V & visitor,
                                            VertexID u)
    {
        { visitor.discover_vertex(u) } -> std::same_as<ControlAction>;
    };

    template <typename V>
    concept ExamineControllable = requires (V & visitor,
                                            VertexID u,
                                            VertexID v,
                                            EdgeID e)
    {
        { visitor.examine_edge(u, v, e) } -> std::same_as<ControlAction>;
    };

    // For dijkstra
    template <typename V>
    concept ExpandControllable = requires (V & visitor,
                                            VertexID u)
    {
        { visitor.expand_vertex(u) } -> std::same_as<ControlAction>;
    };



};
