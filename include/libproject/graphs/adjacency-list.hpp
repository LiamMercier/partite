#pragma once
#include <vector>

#include <libproject/internal/libproject-assert.hpp>
#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{

// TODO <optimization>: this class really needs a way to
//                      allocate using a memory pool.
//
//                      Otherwise, the inner vectors are
//                      all fragmented, very bad since most
//                      graphs are going to be sparse and large.

// TODO: asserts

// AdjacencyList class for simple graphs.
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
    // Ranges front end
    //

    // TODO: neighbors(v) const

    // TODO: in_neighbors(v) const

    // TODO: out_neighbors(v) const

    // TODO: edges() const

    //
    // Edge Operations
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
        libproject_assert(find_edge(from, to) == EdgeID::empty,
                          "AdjacencyList::add_edge inserted "
                          "duplicate edge ("
                          + std::to_string(from.value)
                          + " "
                          + std::to_string(to.value)
                          + ") with ID "
                          + std::to_string(edge_id.value)
                          + " (existing edge has ID "
                          + std::to_string(find_edge(from, to).value)
                          + ")");

        out_list(from).push_back({to, edge_id});
        in_list(to).push_back({from, edge_id});

        if (to == from)
        {
            num_self_loops_ += 1;
        }

        return edge_id;
    }

    // For undirected graphs, use the standard adjacency list structure.
    EdgeID add_edge(VertexID from, VertexID to, EdgeID edge_id)
        requires (!directed)
    {
        libproject_assert(find_edge(from, to) == EdgeID::empty,
                          "AdjacencyList::add_edge inserted "
                          "duplicate edge ("
                          + std::to_string(from.value)
                          + " "
                          + std::to_string(to.value)
                          + ") with ID "
                          + std::to_string(edge_id.value)
                          + " (existing edge has ID "
                          + std::to_string(find_edge(from, to).value)
                          + ")");

        adj_list(from).push_back({to, edge_id});

        // Do not insert self loops twice, otherwise
        // neighbors(v) will return the same value twice.
        if (to != from)
        {
            adj_list(to).push_back({from, edge_id});
        }
        else
        {
            // Count this self loop and record in our array.
            num_self_loops_ += 1;
            self_loop_[from.value] = 1;
        }

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

        if (to == from)
        {
            num_self_loops_ += 1;
        }

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

        // Do not insert self loops twice, otherwise
        // neighbors(v) will return the same value twice.
        if (to != from)
        {
            adj_list(to).push_back({from, edge_id});
        }
        else
        {
            // Count this self loop and record in our array.
            num_self_loops_ += 1;
            self_loop_[from.value] = 1;
        }

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

        if (from == to && e_id != EdgeID::empty)
        {
            num_self_loops_ -= 1;

            libproject_assert(num_self_loops_ + 1 >
                              num_self_loops_,
                              "AdjacencyList::remove_edge underflow was "
                              "detected for num_self_loops_ (likely a"
                              " library error)");
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

        if (from != to)
        {
            for (size_t i = 0; i < to_adj.size(); i++)
            {
                if (to_adj[i].first == from)
                {
                    to_adj[i] = std::move(to_adj.back());
                    to_adj.pop_back();
                    break;
                }
            }
        }
        else if (e_id != EdgeID::empty)
        {
            self_loop_[from.value] = 0;
            num_self_loops_ -= 1;

            libproject_assert(num_self_loops_ + 1 >
                              num_self_loops_,
                              "AdjacencyList::remove_edge underflow was "
                              "detected for num_self_loops_ (likely a"
                              " library error)");
        }

        return e_id;
    }

    //
    // Vertex Operations
    //

    // Expand the list, otherwise do nothing (list already exists).
    bool add_vertex(VertexID v)
        requires (directed)
    {
        libproject_assert(v.value <= out_edges_.size(),
                          "AdjacencyList::add_vertex was called "
                          "on VertexID "
                          + std::to_string(v.value)
                          + " which was expected to be <= "
                          + std::to_string(out_edges_.size())
                          + " (likely a library error)");

        if (v.value == out_edges_.size())
        {
            out_edges_.emplace_back();
            in_edges_.emplace_back();
        }

        return true;
    }

    bool add_vertex(VertexID v)
        requires (!directed)
    {
        libproject_assert(v.value <= adj_.size(),
                          "AdjacencyList::add_vertex was called "
                          "on VertexID "
                          + std::to_string(v.value)
                          + " which was expected to be <= "
                          + std::to_string(adj_.size())
                          + " (likely a library error)");

        if (v.value == adj_.size())
        {
            adj_.emplace_back();
        }

        return true;
    }

    // In a digraph, we insert (u, v) as out[u] = v, in[v] = u
    // so we need to go over the in and out lists.
    //
    // for u : out_list(v)
    //     for a : in_list(u)
    //         if (a == v)
    //             remove a
    //
    // for u : in_list(v)
    //     for a : out_list(u)
    //         if (a == v)
    //             remove a
    //
    // clear in_list(v) and out_list(v)
    //
    // Therefore on average this runs in
    // Theta(deg(v) * average_degree) time (see docs/design).
    void remove_vertex(VertexID v)
        requires (directed)
    {
        for (const auto & [u, _] : out_list(v))
        {
            auto & in = in_list(u.first);

            for (size_t i = 0; i < in.size(); i++)
            {
                // If we found a match, swap and pop.
                if (in[i].first == v)
                {
                    std::swap(in[i], in.back());
                    in.pop_back();
                    break;
                }
            }
        }

        for (const auto & [u, _] : in_list(v))
        {
            auto & out = out_list(u.first);

            for (size_t i = 0; i < out.size(); i++)
            {
                // If we found a match, swap and pop.
                if (out[i].first == v)
                {
                    std::swap(out[i], out.back());
                    out.pop_back();
                    break;
                }
            }
        }

        out_list(v).clear();
        in_list(v).clear();
    }

    // The undirected graph case is easier.
    //
    // For u : adj_list(v)
    //     remove v from adj_list(u)
    //
    // clear adj_list(v)
    void remove_vertex(VertexID v)
        requires (!directed)
    {
        for (const auto & [u, _] : adj_list(v))
        {
            auto & u_list = adj_list(u);

            for (size_t i = 0; i < u_list.size(); i++)
            {
                // swap and pop
                if (u_list[i].first == v)
                {
                    std::swap(u_list[i], u_list.back());
                    u_list.pop_back();
                    break;
                }
            }
        }

        adj_list(v).clear();
    }

    //
    // Utility
    //

    void clear()
        requires (directed)
    {
        for (auto & list : out_edges_)
        {
            list.clear();
        }

        for (auto & list : in_edges_)
        {
            list.clear();
        }
    }

    void clear()
        requires (!directed)
    {
        for (auto & list : adj_)
        {
            list.clear();
        }
    }

    size_t degree(VertexID v) const
        requires (directed)
    {
        return in_degree(v) + out_degree(v);
    }

    size_t degree(VertexID v) const
        requires (!directed)
    {
        // A self loop adds 1 to the degree that our adjacency list
        // would not be able to account for.
        return adj_[v.value].size() + self_loop_[v.value];
    }

    size_t in_degree(VertexID v) const
        requires (directed)
    {
        return in_edges_[v.value].size();
    }

    size_t out_degree(VertexID v) const
        requires (directed)
    {
        return out_edges_[v.value].size();
    }

    bool has_self_loops() const
    {
        return num_self_loops_ > 0;
    }

    size_t num_self_loops() const
    {
        return num_self_loops_;
    }

    // Search the list in Theta(deg(v)) averate time.
    bool has_self_loop(VertexID v) const
        requires (directed)
    {
        for (const auto & u : out_list(v))
        {
            if (u.first == v)
            {
                return true;
            }
        }

        return false;
    }

    // Use the lookup table, we already have to store this
    // information for our degree checks.
    bool has_self_loop(VertexID v) const
        requires (!directed)
    {
        return self_loop_[v.value];
    }

    // TODO: valid_vertex(VertexID v)

    // TODO: vertex_count()

    //
    // Transformations
    //

    // TODO: compute_transpose()

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

    // State storage for self loops in an undirected graph.
    //
    // We can either store this or the degree, and this is cheaper
    // while also reducing our cost for has_self_loop(v)
    std::conditional_t<!directed,
                       std::vector<uint8_t>,
                       std::monostate> self_loop_;

    // We need to count this on add and remove, or we are forced
    // to do Theta(V) average time iteration when it is requested.
    size_t num_self_loops_{0};
};

};
