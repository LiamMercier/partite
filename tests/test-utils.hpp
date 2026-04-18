#pragma once

#include <tuple>
#include <unordered_set>

#include <libproject/graphs/graph-constants.hpp>

using namespace libproject::graphs;

template <typename GraphT, typename EdgeDataT, typename VertexDataT>
struct GraphConfig
{
    using Graph = GraphT;
    using EdgeData = EdgeDataT;
    using VertexData = VertexDataT;
};

struct PairHash
{
    std::size_t operator()(const std::pair<size_t, size_t> & pair) const
    {
        return std::hash<size_t>()(pair.first)
               ^ std::hash<size_t>()(pair.second);
    }
};

struct VertexIDHash
{
    std::size_t operator()(const VertexID & v) const
    {
        return std::hash<size_t>()(v.value);
    }
};

struct EdgeIDHash
{
    std::size_t operator()(const EdgeID & e) const
    {
        return std::hash<size_t>()(e.value);
    }
};

using VertexIDSet = std::unordered_set<VertexID, VertexIDHash>;
using EdgeIDSet = std::unordered_set<EdgeID, EdgeIDHash>;
