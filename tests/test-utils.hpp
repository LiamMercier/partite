#pragma once

#include <tuple>

struct PairHash
{
    std::size_t operator()(const std::pair<size_t, size_t> & pair) const
    {
        return std::hash<size_t>()(pair.first)
               ^ std::hash<size_t>()(pair.second);
    }
};

template <typename GraphT, typename EdgeDataT, typename VertexDataT>
struct GraphConfig
{
    using Graph = GraphT;
    using EdgeData = EdgeDataT;
    using VertexData = VertexDataT;
};
