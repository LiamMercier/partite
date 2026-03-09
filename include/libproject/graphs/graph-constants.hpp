#pragma once
#include <cstdint>

#ifndef GRAPH_VERTEX_ID_TYPE
#define GRAPH_VERTEX_ID_TYPE uint32_t
#endif

#ifndef GRAPH_EDGE_ID_TYPE
#define GRAPH_EDGE_ID_TYPE uint32_t
#endif

namespace libproject::graphs
{
    struct VertexID
    {
        VertexID(GRAPH_VERTEX_ID_TYPE v)
        :value(v)
        {}

        constexpr auto operator<=>(const VertexID & other) const = default;

        GRAPH_VERTEX_ID_TYPE value;
    };

    struct EdgeID
    {
        EdgeID(GRAPH_EDGE_ID_TYPE v)
        :value(v)
        {}

        constexpr auto operator<=>(const EdgeID & other) const = default;

        GRAPH_EDGE_ID_TYPE value;
    };
};
