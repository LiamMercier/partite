#pragma once

#include <libproject/graphs/graph-constants.hpp>

// Definition of a graph topology
concept GraphRepresentation = requires(T t, VertexID v_id, EdgeID e_id)
{
    // TODO: probably just require we can fetch neighbors
}
