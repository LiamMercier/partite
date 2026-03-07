#pragma once
#include <cstdint>

#ifndef GRAPH_VERTEX_ID_TYPE
#define GRAPH_VERTEX_ID_TYPE uint32_t
#endif

#ifndef GRAPH_EDGE_ID_TYPE
#define GRAPH_EDGE_ID_TYPE uint32_t
#endif

namespace libproject::Graphs
{
    using vertex_id = GRAPH_VERTEX_ID_TYPE;
    using edge_id = GRAPH_EDGE_ID_TYPE;
};
