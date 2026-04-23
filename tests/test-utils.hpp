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

struct IncidentEdgeHash
{
    std::size_t operator()(const IncidentEdge & edge) const
    {
        return std::hash<size_t>()(edge.neighbor.value)
               ^ std::hash<size_t>()(edge.id.value);
    }
};

struct IncidentEdgeEqual
{
    bool operator()(const IncidentEdge & a, const IncidentEdge & b) const
    {
        return (a.neighbor.value == b.neighbor.value)
               && (a.id == b.id);
    }
};

struct EdgeViewHash
{
    std::size_t operator()(const EdgeView & ev) const
    {
        return std::hash<size_t>()(ev.from.value)
               ^ std::hash<size_t>()(ev.to.value)
               ^ std::hash<size_t>()(ev.id.value);
    }
};

struct EdgeViewEqual
{
    bool operator()(const EdgeView & a, const EdgeView & b) const
    {
        return (a.from == b.from)
               && (a.to == b.to)
               && (a.id == b.id);
    }
};

using VertexIDSet = std::unordered_set<VertexID, VertexIDHash>;
using EdgeIDSet = std::unordered_set<EdgeID, EdgeIDHash>;
using IncidentEdgeSet = std::unordered_set<IncidentEdge,
                                           IncidentEdgeHash,
                                           IncidentEdgeEqual>;
using EdgeViewSet = std::unordered_set<EdgeView, EdgeViewHash, EdgeViewEqual>;
