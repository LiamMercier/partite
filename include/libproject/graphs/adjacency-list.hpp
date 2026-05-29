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


// AdjacencyList class for simple graphs.
template <RepresentationConfig Config>
class AdjacencyList
{
public:
    static constexpr bool directed = Config.Directed;
    static constexpr bool multigraph = Config.Multigraph;

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
    :adj_(num_vertices),
    self_loop_(num_vertices)
    {
    }

    // Bulk edge constructor.
    template <EdgeViewRange ER>
    AdjacencyList(size_t num_vertices, ER&& edge_views)
    :AdjacencyList(num_vertices)
    {
        build(std::forward<ER>(edge_views));
    }

    template <EdgeViewRange ER>
    void rebuild(ER&& edge_views)
    {
        clear();
        build(std::forward<ER>(edge_views));
    }

    //
    // Ranges front end
    //

    //
    // Views for getting the VertexID of each neighbor
    //

    decltype(auto) neighbors(VertexID v) const
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::neighbors(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        if constexpr(directed)
        {
            return out_neighbors(v);
        }
        else
        {
            auto & adj = adj_list(v);
            return adj
                   | std::views::transform([](const auto & edge){
                       return edge.first;
                   });
        }
    }

    decltype(auto) out_neighbors(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::out_neighbors(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        auto & out = out_list(v);
        return out
               | std::views::transform([](const auto & edge){
                   return edge.first;
               });
    }

    decltype(auto) in_neighbors(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::in_neighbors(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        auto & in = in_list(v);
        return in
               | std::views::transform([](const auto & edge){
                   return edge.first;
               });
    }

    //
    // Views for getting the IncidentEdge descriptor of each neighbor
    //

    decltype(auto) incident_edges(VertexID v) const
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::incident_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        if constexpr(directed)
        {
            return out_edges(v);
        }
        else
        {
            auto & adj = adj_list(v);
            return adj
                   | std::views::transform([](const auto & edge){
                       return IncidentEdge{edge.first, edge.second};
                   });
        }
    }

    decltype(auto) out_edges(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::out_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        auto & out = out_list(v);
        return out
               | std::views::transform([](const auto & edge){
                   return IncidentEdge{edge.first, edge.second};
               });
    }

    decltype(auto) in_edges(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::in_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        auto & in = in_list(v);
        return in
               | std::views::transform([](const auto & edge){
                   return IncidentEdge{edge.first, edge.second};
               });
    }

    //
    // Views for getting the EdgeID of each neighbor
    //

    decltype(auto) incident_edge_ids(VertexID v) const
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::incident_edge_ids(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        if constexpr(directed)
        {
            return out_edge_ids(v);
        }
        else
        {
            auto & adj = adj_list(v);
            return adj
                   | std::views::transform([](const auto & edge){
                       return edge.second;
                   });
        }
    }

    decltype(auto) out_edge_ids(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::out_edge_ids(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        auto & out = out_list(v);
        return out
               | std::views::transform([](const auto & edge){
                   return edge.second;
               });
    }

    decltype(auto) in_edge_ids(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::in_edge_ids(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        auto & in = in_list(v);
        return in
               | std::views::transform([](const auto & edge){
                   return edge.second;
               });
    }

    //
    // Edge Operations
    //

    EdgeID find_edge(VertexID from, VertexID to) const noexcept
        requires (!multigraph && directed)
    {
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::find_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::find_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

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
        requires (!multigraph && !directed)
    {
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::find_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::find_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

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

    decltype(auto) find_edges(VertexID from, VertexID to) const
        requires (multigraph && directed)
    {
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::find_edges(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::find_edges(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

        return out_edges_[from.value]
               | std::views::filter([this, to](auto ie){
                    return ie.first == to;
                })
               | std::views::transform([this](auto ie){
                    return ie.second;
                });
    }

    decltype(auto) find_edges(VertexID from, VertexID to) const
        requires (multigraph && !directed)
    {
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::find_edges(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::find_edges(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

        return adj_[from.value]
               | std::views::filter([this, to](auto ie){
                    return ie.first == to;
                })
               | std::views::transform([this](auto ie){
                    return ie.second;
                });
    }

    bool has_edge(VertexID from, VertexID to) const noexcept
        requires (directed)
    {
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::has_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::has_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

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
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::has_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::has_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

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
        if constexpr (!multigraph)
        {
            libproject_assert(find_edge(from, to) == EdgeID::empty,
                              "AdjacencyList::add_edge(from, to, edge_id) "
                              "tried to insert duplicate edge; from = "
                              + std::to_string(from.value)
                              + ", to = "
                              + std::to_string(to.value)
                              + ", edge_id = "
                              + std::to_string(edge_id.value)
                              + " (existing edge has ID "
                              + std::to_string(find_edge(from, to).value)
                              + ")");
        }

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
        if constexpr (!multigraph)
        {
            libproject_assert(find_edge(from, to) == EdgeID::empty,
                              "AdjacencyList::add_edge(from, to, edge_id) "
                              "tried to insert duplicate edge; from = "
                              + std::to_string(from.value)
                              + ", to = "
                              + std::to_string(to.value)
                              + ", edge_id = "
                              + std::to_string(edge_id.value)
                              + " (existing edge has ID "
                              + std::to_string(find_edge(from, to).value)
                              + ")");
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

            if constexpr (!multigraph)
            {
                self_loop_[from.value] = 1;
            }
            else
            {
                self_loop_[from.value] += 1;
            }
        }

        return edge_id;
    }

    // Ensure the edge being added is unique
    EdgeID add_edge_unique(VertexID from, VertexID to, EdgeID edge_id)
        requires (!multigraph && directed)
    {
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::add_edge_unique(from, to, edge_id): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::add_edge_unique(from, to, edge_id): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

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
        requires (!multigraph && !directed)
    {
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::add_edge_unique(from, to, edge_id): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::add_edge_unique(from, to, edge_id): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

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
        requires (!multigraph && directed)
    {
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::remove_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::remove_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

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
                              "AdjacencyList::remove_edge(from, to): "
                              "internal library error: num_self_loops_ "
                              "underflowed");
        }

        return e_id;
    }

    EdgeID remove_edge(VertexID from, VertexID to) noexcept
        requires (!multigraph && !directed)
    {
        libproject_assert(valid_vertex(from),
                          "AdjacencyList::remove_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "AdjacencyList::remove_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

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
            if constexpr (!multigraph)
            {
                self_loop_[from.value] = 0;
            }
            else
            {
                self_loop_[from.value] -= 1;
            }

            num_self_loops_ -= 1;

            libproject_assert(num_self_loops_ + 1 >
                              num_self_loops_,
                              "AdjacencyList::remove_edge(from, to): "
                              "internal library error: num_self_loops_ "
                              "underflowed");
        }

        return e_id;
    }

    // TODO: edge ID edge removal

    //
    // Vertex Operations
    //

    // Expand the list, otherwise do nothing (list already exists).
    bool add_vertex(VertexID v)
        requires (directed)
    {
        libproject_assert(v.value <= out_edges_.size(),
                          "AdjacencyList::add_vertex(v): "
                          "internal library error: monotonic VertexID "
                          "is larger than list size; v = "
                          + std::to_string(v.value)
                          + ", out_edges_.size() = "
                          + std::to_string(out_edges_.size()));

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
                          "AdjacencyList::add_vertex(v): "
                          "internal library error: monotonic VertexID "
                          "is larger than list size; v = "
                          + std::to_string(v.value)
                          + ", adj_.size() = "
                          + std::to_string(adj_.size()));

        if (v.value == adj_.size())
        {
            adj_.emplace_back();
            self_loop_.emplace_back(0);
        }

        return true;
    }

    // TODO: multigraph setup for vertex removals.

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
    template <typename EdgeRemovalCallback>
    void remove_vertex(VertexID v, EdgeRemovalCallback on_edge_removed)
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::remove_vertex(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        EdgeID self_loop_id = EdgeID::empty;

        for (const auto & [u, e_id] : out_list(v))
        {
            // Skip self loops for now.
            if (u == v)
            {
                self_loop_id = e_id;
                continue;
            }

            auto & in = in_list(u);

            for (size_t i = 0; i < in.size(); i++)
            {
                // If we found a match, swap and pop.
                if (in[i].first == v)
                {
                    on_edge_removed(in[i].second);
                    std::swap(in[i], in.back());
                    in.pop_back();
                    break;
                }
            }
        }

        for (const auto & [u, _] : in_list(v))
        {
            // Skip self loops for now.
            if (u == v)
            {
                continue;
            }

            auto & out = out_list(u);

            for (size_t i = 0; i < out.size(); i++)
            {
                // If we found a match, swap and pop.
                if (out[i].first == v)
                {
                    on_edge_removed(out[i].second);
                    std::swap(out[i], out.back());
                    out.pop_back();
                    break;
                }
            }
        }

        if (self_loop_id != EdgeID::empty)
        {
            num_self_loops_ -= 1;
            libproject_assert(num_self_loops_ + 1 >
                              num_self_loops_,
                              "AdjacencyList::remove_vertex(v): "
                              "internal library error: num_self_loops_ "
                              "underflowed");

            on_edge_removed(self_loop_id);
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
    template <typename EdgeRemovalCallback>
    void remove_vertex(VertexID v, EdgeRemovalCallback on_edge_removed)
        requires (!directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::remove_vertex(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        EdgeID self_loop_id = EdgeID::empty;

        for (const auto & [u, e_id] : adj_list(v))
        {
            auto & u_list = adj_list(u);

            if (u == v)
            {
                self_loop_id = e_id;
                continue;
            }

            for (size_t i = 0; i < u_list.size(); i++)
            {
                // swap and pop
                if (u_list[i].first == v)
                {
                    on_edge_removed(u_list[i].second);
                    std::swap(u_list[i], u_list.back());
                    u_list.pop_back();
                    break;
                }
            }
        }

        if (self_loop_id != EdgeID::empty)
        {
            num_self_loops_ -= 1;
            libproject_assert(num_self_loops_ + 1 >
                              num_self_loops_,
                              "AdjacencyList::remove_vertex(v): "
                              "internal library error: num_self_loops_ "
                              "underflowed");

            on_edge_removed(self_loop_id);
        }

        adj_list(v).clear();
        self_loop_[v.value] = 0;
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

        num_self_loops_ = 0;
    }

    void clear()
        requires (!directed)
    {
        for (auto & list : adj_)
        {
            list.clear();
        }

        // Quickly fill the vector with element 0.
        std::fill(self_loop_.begin(), self_loop_.end(), 0);
        num_self_loops_ = 0;
    }

    size_t degree(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::degree(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return in_degree(v) + out_degree(v);
    }

    size_t degree(VertexID v) const
        requires (!directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::degree(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        // A self loop adds 1 to the degree that our adjacency list
        // would not be able to account for.
        return adj_[v.value].size() + self_loop_[v.value];
    }

    size_t in_degree(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::in_degree(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return in_edges_[v.value].size();
    }

    size_t out_degree(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::out_degree(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

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

    // Search the list in Theta(deg(v)) average time.
    bool has_self_loop(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::has_self_loop(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

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
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::has_self_loop(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return self_loop_[v.value] > 0;
    }

    bool valid_vertex(VertexID v) const
    {
        return v.value < max_vertex_id();
    }

    size_t max_vertex_id() const noexcept
        requires (directed)
    {
        return out_edges_.size();
    }

    size_t max_vertex_id() const noexcept
        requires (!directed)
    {
        return adj_.size();
    }

    //
    // Transformations
    //

    [[nodiscard]] AdjacencyList compute_transpose() const
    {
        if constexpr (directed)
        {
            // Swap the adjacency lists.
            return AdjacencyList{in_edges_,
                                 out_edges_,
                                 num_self_loops_};
        }
        else
        {
            // Return a copy, G^T = G in an undirected graph.
            return AdjacencyList{adj_,
                                 self_loop_,
                                 num_self_loops_};
        }
    }

    void transpose_inplace()
    {
        if constexpr (directed)
        {
            // Swap the adjacency lists.
            std::swap(in_edges_, out_edges_);
        }
        else
        {
            // G^T = G in an undirected graph.
            return;
        }
    }

private:
    template <EdgeViewRange ER>
    void build(ER&& edge_views)
        requires (directed)
    {
        // First pass, count the degree of each vertex.
        size_t v_count = out_edges_.size();

        std::vector<size_t> in_degrees(v_count, 0);
        std::vector<size_t> out_degrees(v_count, 0);

        for (auto&& ev: edge_views)
        {
            libproject_assert(valid_vertex(ev.from),
                              "AdjacencyList::build(edge_views): "
                              "invalid VertexID; from = "
                              + std::to_string(ev.from.value));

            libproject_assert(valid_vertex(ev.to),
                              "AdjacencyList::build(edge_views): "
                              "invalid VertexID; to = "
                              + std::to_string(ev.to.value));

            out_degrees[ev.from.value] += 1;
            in_degrees[ev.to.value] += 1;
        }

        // Now that we know the degree of each list, reserve.
        for (size_t i = 0; i < v_count; i++)
        {
            out_edges_[i].reserve(out_degrees[i]);
            in_edges_[i].reserve(in_degrees[i]);
        }

        // Do a second pass over the view and insert.
        for (auto&& ev : edge_views)
        {
            out_list(ev.from).push_back({ev.to, ev.id});
            in_list(ev.to).push_back({ev.from, ev.id});

            if (ev.from == ev.to)
            {
                num_self_loops_ += 1;
            }
        }
    }

    template <EdgeViewRange ER>
    void build(ER&& edge_views)
        requires (!directed)
    {
        // First pass, count the degree of each vertex.
        size_t v_count = adj_.size();

        std::vector<size_t> degrees(v_count, 0);

        for (auto&& ev: edge_views)
        {
            libproject_assert(valid_vertex(ev.from),
                              "AdjacencyList::build(edge_views): "
                              "invalid VertexID; from = "
                              + std::to_string(ev.from.value));

            degrees[ev.from.value] += 1;

            // Prevent double counting since we do not store
            // multiple copies of the same edge and thus do not
            // need to allocate this extra space.
            if (ev.from != ev.to)
            {
                libproject_assert(valid_vertex(ev.to),
                                  "AdjacencyList::build(edge_views): "
                                  "invalid VertexID; to = "
                                  + std::to_string(ev.to.value));

                degrees[ev.to.value] += 1;
            }
        }

        // Now that we know the degree of each list, reserve.
        for (size_t i = 0; i < v_count; i++)
        {
            adj_[i].reserve(degrees[i]);
        }

        // Do a second pass over the view and insert.
        for (auto&& ev : edge_views)
        {
            adj_list(ev.from).push_back({ev.to, ev.id});

            if (ev.from != ev.to)
            {
                adj_list(ev.to).push_back({ev.from, ev.id});
            }
            else
            {
                num_self_loops_ += 1;

                if constexpr (!multigraph)
                {
                    self_loop_[ev.from.value] = 1;
                }
                else
                {
                    self_loop_[ev.from.value] += 1;
                }
            }
        }
    }

    auto & out_list(VertexID v)
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::out_list(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return out_edges_[v.value];
    }

    const auto & out_list(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::out_list(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return out_edges_[v.value];
    }

    auto & in_list(VertexID v)
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::in_list(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return in_edges_[v.value];
    }

    const auto & in_list(VertexID v) const
        requires (directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::in_list(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return in_edges_[v.value];
    }

    auto & adj_list(VertexID v)
        requires (!directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::adj_list(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return adj_[v.value];
    }

    const auto & adj_list(VertexID v) const
        requires (!directed)
    {
        libproject_assert(valid_vertex(v),
                          "AdjacencyList::adj_list(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return adj_[v.value];
    }

private:
    using ListStorage = std::vector<std::vector<std::pair<VertexID, EdgeID>>>;

    // If we have a multigraph, we might have more than uint8_t max self loops.
    //
    // uint32_t is a safe default, we should never expect anything near this.
    using loop_count_type = std::conditional_t<multigraph, uint32_t, uint8_t>;

    // For returning a copy.
    AdjacencyList(ListStorage out_edges,
                  ListStorage in_edges,
                  size_t loop_count)
        requires (directed)
    :out_edges_(std::move(out_edges)),
    in_edges_(std::move(in_edges)),
    num_self_loops_{loop_count}
    {
    }

    AdjacencyList(ListStorage adj,
                  std::vector<loop_count_type> self_loop_state,
                  size_t loop_count)
        requires (!directed)
    :adj_(std::move(adj)),
    self_loop_(std::move(self_loop_state)),
    num_self_loops_{loop_count}
    {
    }

private:
    // If we have a digraph, store out_edges_ and in_edges_ for
    // fast vertex deletion. Otherwise, we can store one adj_
    // for undirected graphs.
    std::conditional_t<
        directed,
        ListStorage,
        std::monostate> out_edges_;

    std::conditional_t<
        directed,
        ListStorage,
        std::monostate> in_edges_;

    std::conditional_t<
        !directed,
        ListStorage,
        std::monostate> adj_;

    // State storage for self loops in an undirected graph.
    //
    // We can either store this or the degree, and this is cheaper
    // while also reducing our cost for has_self_loop(v)
    std::conditional_t<!directed,
                       std::vector<loop_count_type>,
                       std::monostate> self_loop_;

    // We need to count this on add and remove, or we are forced
    // to do Theta(V) average time iteration when it is requested.
    size_t num_self_loops_{0};
};

};
