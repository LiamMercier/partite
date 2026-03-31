#pragma once
#include <vector>

#include <libproject/internal/libproject-assert.hpp>
#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{

// TODO: asserts
template <bool Directed>
class AdjacencyList
{
public:
    static constexpr bool directed = Directed;

public:
    // Create an AdjacencyList with all zero edges.
    AdjacencyList(size_t num_vertices)
        requires (directed)
    :out_edges_(num_vertices),
    in_edges_(num_vertices)
    {
    }

    AdjacencyList(size_t num_vertices)
        requires (!directed)
    :adj_(num_vertices)
    {
    }

    //
    // Edge operations
    //

    bool has_edge(VertexID from, VertexID to) const noexcept
        requires (directed)
    {
        for (const auto & [v, _] : out_edges_[from.value])
        {
            if (v == to)
            {
                return true;
            }
        }

        return false;
    }

    bool has_edge(VertexID from, VertexID to) const noexcept
        requires (!directed)
    {
        for (const auto & [v, _] : adj_[from.value])
        {
            if (v == to)
            {
                return true;
            }
        }

        return false;
    }

    // For digraphs, we add the edge to out_edges_ and in_edges_
    EdgeID add_edge(VertexID from, VertexID to, EdgeID edge_id)
        requires (directed)
    {
        out_edges_[from.value].push_back({to, edge_id});
        in_edges_[to.value].push_back({from, edge_id});
        return edge_id;
    }

    // For undirected graphs, use the standard adjacency list structure.
    EdgeID add_edge(VertexID from, VertexID to, EdgeID edge_id)
        requires (!directed)
    {
        adj_[from.value].push_back({to, edge_id});
        adj_[to.value].push_back({from, edge_id});
        return edge_id;
    }

    // TODO: implement when find_edge is implemented.
    EdgeID add_edge_unique(VertexID from, VertexID to, EdgeID edge_id)
        requires (directed)
    {
        out_edges_[from.value].push_back({to, edge_id});
        in_edges_[to.value].push_back({from, edge_id});
        return edge_id;
    }

    EdgeID add_edge_unique(VertexID from, VertexID to, EdgeID edge_id)
        requires (!directed)
    {
        adj_[from.value].push_back({to, edge_id});
        adj_[to.value].push_back({from, edge_id});
        return edge_id;
    }

private:
    // If we have a digraph, store out_edges_ and in_edges_ for
    // fast vertex deletion. Otherwise, we can store one adj_
    // for undirected graphs.
    std::conditional_t<
        directed,
        std::vector<std::vector<std::pair<VertexID, EdgeID>>>,
        std::monostate> out_edges_;

    std::conditional_t<
        directed,
        std::vector<std::vector<std::pair<VertexID, EdgeID>>>,
        std::monostate> in_edges_;

    std::conditional_t<
        !directed,
        std::vector<std::vector<std::pair<VertexID, EdgeID>>>,
        std::monostate> adj_;
};

};
