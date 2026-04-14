#pragma once
#include <cstdint>

#ifndef GRAPH_VERTEX_ID_TYPE
#define GRAPH_VERTEX_ID_TYPE uint32_t
#endif

#ifndef GRAPH_EDGE_ID_TYPE
#define GRAPH_EDGE_ID_TYPE uint32_t
#endif

// TODO: make this more strict (prevent VertexID <-> EdgeID conversions)
namespace libproject::graphs
{
    struct VertexID
    {
        static constexpr GRAPH_VERTEX_ID_TYPE max = std::numeric_limits<GRAPH_VERTEX_ID_TYPE>::max();
        static constexpr GRAPH_VERTEX_ID_TYPE empty = max;

        VertexID(GRAPH_VERTEX_ID_TYPE v)
        :value(v)
        {}

        VertexID()
        :value(0)
        {}

        constexpr auto operator<=>(const VertexID & other) const = default;

        GRAPH_VERTEX_ID_TYPE value;
    };

    static_assert(std::is_trivially_copyable_v<VertexID>);
    static_assert(sizeof(VertexID) == sizeof(GRAPH_VERTEX_ID_TYPE));

    struct EdgeID
    {
        static constexpr GRAPH_EDGE_ID_TYPE max = std::numeric_limits<GRAPH_EDGE_ID_TYPE>::max();
        static constexpr GRAPH_EDGE_ID_TYPE empty = max;

        EdgeID(GRAPH_EDGE_ID_TYPE v)
        :value(v)
        {}

        EdgeID()
        :value(0)
        {}

        constexpr auto operator<=>(const EdgeID & other) const = default;

        GRAPH_EDGE_ID_TYPE value;
    };

    static_assert(std::is_trivially_copyable_v<EdgeID>);
    static_assert(sizeof(EdgeID) == sizeof(GRAPH_EDGE_ID_TYPE));

    // For global views over all edges
    struct EdgeView
    {
        VertexID from;
        VertexID to;
        EdgeID id;
    };

    static_assert(std::is_trivially_copyable_v<EdgeView>);

    // For views which already have the input vertex
    struct IncidentEdge
    {
        VertexID neighbor;
        EdgeID id;
    };

    static_assert(std::is_trivially_copyable_v<IncidentEdge>);

};
