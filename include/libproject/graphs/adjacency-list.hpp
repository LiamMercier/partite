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

    EdgeID find_edge(VertexID from, VertexID to) const noexcept
        requires (directed)
    {
        // Look through the adjacency list for a pair with "to"
        for (const auto & [v, e_id] : out_edges_[from.value])
        {
            if (v == to)
            {
                return e_id;
            }
        }

        return EdgeID::empty;
    }

    EdgeID find_edge(VertexID from, VertexID to) const noexcept
        requires (!directed)
    {
        // Look through the adjacency list for a pair with "to"
        for (const auto & [v, e_id] : adj_[from.value])
        {
            if (v == to)
            {
                return e_id;
            }
        }

        return EdgeID::empty;
    }

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
        out_list(from).push_back({to, edge_id});
        in_list(to).push_back({from, edge_id});
        return edge_id;
    }

    // For undirected graphs, use the standard adjacency list structure.
    EdgeID add_edge(VertexID from, VertexID to, EdgeID edge_id)
        requires (!directed)
    {
        adj_list(from).push_back({to, edge_id});
        adj_list(to).push_back({from, edge_id});
        return edge_id;
    }

    // Ensure the edge being added is unique
    EdgeID add_edge_unique(VertexID from, VertexID to, EdgeID edge_id)
        requires (directed)
    {
        // Return an edge if it already exists.
        EdgeID lookup = find_edge(from, to);
        if (lookup != EdgeID::empty)
        {
            return lookup;
        }

        out_list(from).push_back({to, edge_id});
        in_list(to).push_back({from, edge_id});
        return edge_id;
    }

    EdgeID add_edge_unique(VertexID from, VertexID to, EdgeID edge_id)
        requires (!directed)
    {
        // Return an edge if it already exists.
        EdgeID lookup = find_edge(from, to);
        if (lookup != EdgeID::empty)
        {
            return lookup;
        }

        adj_list(from).push_back({to, edge_id});
        adj_list(to).push_back({from, edge_id});
        return edge_id;
    }

    // For a digraph we find "to" in out_edges_[from]
    // and find "from" in in_edges_[to]
    EdgeID remove_edge(VertexID from, VertexID to) noexcept
        requires (directed)
    {
        EdgeID e_id = EdgeID::empty;
        auto & out_edges = out_list(from);

        for (size_t i = 0; i < out_edges.size(); i++)
        {
            if (out_edges[i].first == to)
            {
                e_id = out_edges[i].second;
                out_edges[i] = std::move(out_edges.back());
                out_edges.pop_back();
                break;
            }
        }

        auto & in_edges = in_list(to);

        for (size_t i = 0; i < in_edges.size(); i++)
        {
            if (in_edges[i].first == from)
            {
                in_edges[i] = std::move(in_edges.back());
                in_edges.pop_back();
                break;
            }
        }

        return e_id;
    }

    EdgeID remove_edge(VertexID from, VertexID to) noexcept
        requires (!directed)
    {
        EdgeID e_id = EdgeID::empty;
        auto & from_adj = adj_list(from);
        auto & to_adj = adj_list(to);

        for (size_t i = 0; i < from_adj.size(); i++)
        {
            if (from_adj[i].first == to)
            {
                e_id = from_adj[i].second;
                from_adj[i] = std::move(from_adj.back());
                from_adj.pop_back();
                break;
            }
        }

        for (size_t i = 0; i < to_adj.size(); i++)
        {
            if (to_adj[i].first == from)
            {
                to_adj[i] = std::move(to_adj.back());
                to_adj.pop_back();
                break;
            }
        }

        return e_id;
    }

private:
    auto & out_list(VertexID v)
        requires (directed)
    {
        return out_edges_[v.value];
    }

    const auto & out_list(VertexID v) const
        requires (directed)
    {
        return out_edges_[v.value];
    }

    auto & in_list(VertexID v)
        requires (directed)
    {
        return in_edges_[v.value];
    }

    const auto & in_list(VertexID v) const
        requires (directed)
    {
        return in_edges_[v.value];
    }

    auto & adj_list(VertexID v)
        requires (!directed)
    {
        return adj_[v.value];
    }

    const auto & adj_list(VertexID v) const
        requires (!directed)
    {
        return adj_[v.value];
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
