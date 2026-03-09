#pragma once

#include <variant>

#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>

template <GraphRepresentation Representation,
          typename VertexData = std::monostate,
          typename EdgeData = std::monostate,
          bool Directed = true>
class PropertyGraph
{
private:
    Representation representation;
    std::vector<VertexData> vertices;
    std::vector<EdgeData> edges;
};
