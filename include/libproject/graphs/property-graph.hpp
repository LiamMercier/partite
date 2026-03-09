#pragma once

#include <variant>

#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>

namespace libproject::graphs
{

template <GraphRepresentation Representation,
          typename VertexData = std::monostate,
          typename EdgeData = std::monostate>
class PropertyGraph
{
public:
    static constexpr bool is_directed = Representation::is_directed;

private:
    Representation representation_;
    std::vector<VertexData> vertices_;
    std::vector<EdgeData> edges_;
};

};
