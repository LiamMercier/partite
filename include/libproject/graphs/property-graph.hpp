#pragma once

#include <variant>

#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>

namespace libproject::graphs
{

template <typename EdgeData = std::monostate>
struct EdgeInput
{
    // Construct an EdgeInput from something like
    // {VertexID, VertexID}
    EdgeInput(VertexID f, VertexID t)
        requires (std::is_same_v<EdgeData, std::monostate>)
    :from(f),
    to(t)
    {}

    // Construct an EdgeInput from something like
    // {VertexID, VertexID}
    EdgeInput(VertexID f, VertexID t, EdgeData d)
        requires (!std::is_same_v<EdgeData, std::monostate>)
    :from(f),
    to(t),
    data(std::move(d))
    {}

    // Construct an EdgeInput from something like
    // {{VertexID, VertexID}, EdgeData}
    //
    // Example: {{0, 1}, "edge value"}
    EdgeInput(std::pair<VertexID, VertexID> verts, EdgeData e)
    :from(verts.first),
    to(verts.second),
    data(std::move(e))
    {
    }

    // Construct an EdgeInput from something like
    // {VertexID, VertexID, EdgeData}
    EdgeInput(std::tuple<VertexID, VertexID, EdgeData> edge)
        requires (!std::is_same_v<EdgeData, std::monostate>)
    :from(std::get<0>(edge)),
    to(std::get<1>(edge)),
    data(std::get<2>(edge))
    {
    }

public:
    VertexID from;
    VertexID to;

    // This might be monostate, if so, don't store memory.
    [[no_unique_address]] EdgeData data{};
};

template <>
struct EdgeInput<std::monostate>
{
    EdgeInput(VertexID f, VertexID t)
    :from(f),
    to(t)
    {}

public:
    VertexID from;
    VertexID to;
};

// TODO: debug mode asserts
template <GraphRepresentation Representation,
          typename EdgeData = std::monostate,
          typename VertexData = std::monostate>
class PropertyGraph
{
public:
    struct EdgeView
    {
        VertexID from;
        VertexID to;
        EdgeID id;
    };

public:
    static constexpr bool is_directed = Representation::is_directed;

    // Empty graph constructor
    PropertyGraph(size_t num_vertices)
        requires PropertyGraphConstructible<Representation>
    :representation_(num_vertices),
    vertices_(num_vertices)
    {
    }

    // Constructor for bulk edge descriptors.
    //
    // In the general case, there is no way for us to optimize
    // construction without knowing the representation. We call
    // add_edge for each edge in the list to build the graph.
    //
    // Asymptotically, the expected time is Theta(|E| * add_edge(e))
    // so if add_edge(e) is expected to take Theta(1) time, there is no
    // asymptotic increase over reading the graph memory.
    //
    // TODO <optimization>: If we find a representation specific optimization
    //                      for construction we can use constexpr to call
    //                      the specific optimization for that representation.
    template <std::ranges::input_range EdgeRange>
    PropertyGraph(size_t num_vertices, EdgeRange && edge_range)
        requires PropertyGraphConstructible<Representation>
    :representation_(num_vertices),
    vertices_(num_vertices)
    {
        // First, lets reserve.
        if constexpr (std::ranges::sized_range<EdgeRange>)
        {
            size_t expected_edges = std::ranges::size(edge_range);
            edges_.reserve(expected_edges);
        }

        for (auto && item : edge_range)
        {
            if constexpr (std::is_same_v<EdgeData, std::monostate>)
            {
                const auto & [from, to] = item;
                add_edge(from, to);
            }
            else
            {
                auto && [from, to, data] = item;
                add_edge(from, to, std::forward<decltype(data)>(data));
            }
        }
    }

    // Cosntruct a graph from a list of verices and a list of edges.
    template <std::ranges::sized_range VertexRange,
              std::ranges::input_range EdgeRange>
    PropertyGraph(VertexRange && vertex_range, EdgeRange && edge_range)
        requires PropertyGraphConstructible<Representation>
    :representation_(std::ranges::size(vertex_range)),
    vertices_(std::ranges::begin(vertex_range), std::ranges::end(vertex_range))
    {
        // First, lets reserve.
        if constexpr (std::ranges::sized_range<EdgeRange>)
        {
            size_t expected_edges = std::ranges::size(edge_range);
            edges_.reserve(expected_edges);
        }

        // TODO: write a debug build assert that edges do not reference
        //       vertices out of the range size given.
        for (auto && item : edge_range)
        {
            if constexpr (std::is_same_v<EdgeData, std::monostate>)
            {
                const auto & [from, to] = item;
                add_edge(from, to);
            }
            else
            {
                auto && [from, to, data] = item;
                add_edge(from, to, std::forward<decltype(data)>(data));
            }
        }
    }

    // TODO <feature>: some sort of from_edges(|V|, edges) that assumes
    //       graph is empty? Or, some other way of transforming an
    //       empty graph to be the desired graph that performs faster?

    //
    // Iteration functions
    //

    // Iterate over all vertices
    auto vertices() const
    {
        return std::views::iota(size_t(0),
                                vertex_count())
               | std::views::transform([](size_t indx){
                   return VertexID{indx};
            });
    }

    // TODO: lets consider neighbors<direction> for generic code without
    //       built in assumptions that neighbors(v) = out_neighbors(v)
    //       in a digraph.

    // Return a range of VertexID values.
    //
    // The representation_ instance streams EdgeID values
    // that are valid for us to iterate over.
    auto neighbors(VertexID v) const
        noexcept(noexcept(out_neighbors(v)))
        requires (is_directed) && OutNeighborsQueryable<Representation>
    {
        return out_neighbors(v);
    }

    auto neighbors(VertexID v) const
        noexcept(noexcept(representation_.neighbors(v)))
        requires (!is_directed) && NeighborsQueryable<Representation>
    {
        // For an undirected graph, we want all edges with
        // v as an element.
        //
        // Return edges_.to[indx] or edges_.from[indx]
        // depending on which vertex is not equal to v.
        return representation_.neighbors(v)
                | std::views::transform([this, v](EdgeID id){
                    return (edges_.to[id.value] == v)
                            ? (edges_.from[id.value])
                            : (edges_.to[id.value]);
                });
    }

    // Return a range of VertexID values for each edge (v, u).
    auto out_neighbors(VertexID v) const
        noexcept(noexcept(representation_.out_neighbors(v)))
        requires (is_directed) && OutNeighborsQueryable<Representation>
    {
        return representation_.out_neighbors(v)
                   | std::views::transform([this](EdgeID id){
                       return edges_.to[id.value];
                   });
    }

    // Return a range of VertexID values for each edge (u, v).
    auto in_neighbors(VertexID v) const
        noexcept(noexcept(representation_.in_neighbors(v)))
        requires (is_directed) && InNeighborsQueryable<Representation>
    {
        return representation_.in_neighbors(v)
                   | std::views::transform([this](EdgeID id){
                       return edges_.from[id.value];
                   });
    }

    // Return a range of EdgeID values for the vertice neighbors.
    //
    // The representation_ instance streams EdgeID values
    // that are valid, so we can return this directly.
    auto edges(VertexID v) const
        noexcept(noexcept(out_edges(v)))
        requires(is_directed) && OutNeighborsQueryable<Representation>
    {
        // For a digraph, we want the outgoing edges.
        return out_edges(v);
    }

    // TODO: should we maybe expose edge views as edges and this as edge_ids

    // TODO: solve consistency issues between
    //       out_X calling neighbors vs out_neighbors (IRL diagram)

    auto edges(VertexID v) const
        noexcept(noexcept(representation_.neighbors(v)))
        requires(!is_directed) && NeighborsQueryable<Representation>
    {
        return representation_.neighbors(v);
    }

    // Iterate over every edge in the graph.
    auto edges() const
        noexcept(noexcept(representation_.edges()))
        requires (GlobalEdgesQueryable<Representation>)
    {
        return representation_.edges();
    }

    // Return a range of EdgeID values for each edge (v, u).
    auto out_edges(VertexID v) const
        noexcept(noexcept(representation_.neighbors(v)))
        requires (is_directed) && OutNeighborsQueryable<Representation>
    {
        return representation_.neighbors(v);
    }

    // Return a range of EdgeID values for each edge (u, v).
    auto in_edges(VertexID v) const
        noexcept(noexcept(representation_.in_neighbors(v)))
        requires (is_directed) && InNeighborsQueryable<Representation>
    {
        return representation_.in_neighbors(v);
    }

    // Return a range of EdgeView values.
    //
    // The representation_ instance streams EdgeID values
    // that are valid, so we can return the edge views
    // from the EdgeID values.
    auto edge_views(VertexID v) const
        noexcept(noexcept(out_edge_views(v)))
        requires (is_directed) && OutNeighborsQueryable<Representation>
    {
        return out_edge_views(v);
    }

    auto edge_views(VertexID v) const
        noexcept(
            noexcept(representation_.neighbors(v)) &&
            noexcept(EdgeView{edges_.from[EdgeID{}.value],
                              edges_.to[EdgeID{}.value],
                              EdgeID{}})
        )
        requires (!is_directed) && NeighborsQueryable<Representation>
    {
        // For an undirected graph, we want all edges with
        // v as an element.
        //
        // Return edges_.to[indx] or edges_.from[indx]
        // depending on which vertex is not equal to v.
        return representation_.neighbors(v)
                | std::views::transform([this](EdgeID id){
                    return EdgeView{edges_.from[id.value],
                                    edges_.to[id.value],
                                    id};
                });
    }

    auto out_edge_views(VertexID v) const
        noexcept(
            noexcept(representation_.neighbors(v)) &&
            noexcept(EdgeView{edges_.from[EdgeID{}.value],
                              edges_.to[EdgeID{}.value],
                              EdgeID{}})
        )
        requires (is_directed) && OutNeighborsQueryable<Representation>
    {
        return representation_.out_neighbors(v)
               | std::views::transform([this](EdgeID id){
                    return EdgeView{edges_.from[id.value],
                                    edges_.to[id.value],
                                    id};
               });
    }

    auto in_edge_views(VertexID v) const
        noexcept(
            noexcept(representation_.in_neighbors(v)) &&
            noexcept(EdgeView{edges_.from[EdgeID{}.value],
                              edges_.to[EdgeID{}.value],
                              EdgeID{}})
        )
        requires (is_directed) && InNeighborsQueryable<Representation>
    {
        return representation_.in_neighbors(v)
               | std::views::transform([this](EdgeID id){
                   return EdgeView{edges_.from[id.value],
                                   edges_.to[id.value],
                                   id};
               });
    }

    //
    // Edge operations
    //

    // By default, return the EdgeID
    EdgeID find_edge(VertexID from, VertexID to) const
        noexcept(noexcept(representation_.find_edge(from, to)))
        requires EdgeQueryable<Representation>
    {
        return representation_.find_edge(from, to);
    }

    // Given an edge ID, find the vertices such that (u, v) has this ID.
    std::pair<VertexID, VertexID> get_edge_pair(EdgeID e_id) const noexcept
    {
        return {edges_.from[e_id.value], edges_.to[e_id.value]};
    }

    // For returning a reference to the EdgeData of an edge.
    EdgeData & get_edge_data(EdgeID e_id) noexcept
        requires (!std::is_same_v<EdgeData, std::monostate>)
    {
        return edges_.data[e_id.value];
    }

    // For copying the EdgeData of an edge.
    const EdgeData & get_edge_data(EdgeID e_id) const noexcept
        requires (!std::is_same_v<EdgeData, std::monostate>)
    {
        return edges_.data[e_id.value];
    }

    VertexData & get_vertex_data(VertexID v_id) noexcept
        requires (!std::is_same_v<VertexData, std::monostate>)
    {
        return vertices_[v_id.value];
    }

    const VertexData & get_vertex_data(VertexID v_id) const noexcept
        requires (!std::is_same_v<VertexData, std::monostate>)
    {
        return vertices_[v_id.value];
    }

    bool has_edge(VertexID from, VertexID to) const
        noexcept(noexcept(representation_.has_edge(from, to)))
        requires EdgeQueryable<Representation>
    {
        return representation_.has_edge(from, to);
    }

    template<typename... Args>
    bool add_edge(VertexID from, VertexID to, Args&&... args)
        requires (std::is_same_v<EdgeData, std::monostate>
                  && sizeof...(Args) == 0) ||
                 (!std::is_same_v<EdgeData, std::monostate>
                  && std::constructible_from<EdgeData, Args...>)
                 && EdgeMutable<Representation>
    {
        EdgeID e_id;

        // If we have deleted an edge in the past, use the old slot.
        if (!free_edges_.empty())
        {
            e_id = free_edges_.back();
            free_edges_.pop_back();

            bool success = representation_.add_edge(from, to, e_id);

            // Undo bad modifications.
            if (!success)
            {
                free_edges_.push_back(e_id);
                return success;
            }

            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                edges_.data[e_id.value] = EdgeData{std::forward<Args>(args)...};
            }

            edges_.from[e_id.value] = from;
            edges_.to[e_id.value] = to;
        }
        // Otherwise, entire vector is full, add a new edge.
        else
        {
            e_id = edges_.size();

            // Push back now, if we throw (bad allocation) then representation_
            // is not corrupted and recovery could be possible.
            //
            // Though, we would need to worry about edges_.from being size N + 1
            // and edges_.to being size N depending on when we throw.
            //
            // TODO <feautre>: function the user can call to check that
            //                 each array is of equal size so they can be fixed.
            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                (edges_.data).emplace_back(std::forward<Args>(args)...);
            }

            (edges_.from).push_back(from);
            (edges_.to).push_back(to);

            bool success = representation_.add_edge(from, to, e_id);

            // Undo invalid modifications.
            if (!success)
            {
                if constexpr (!std::is_same_v<EdgeData, std::monostate>)
                {
                    (edges_.data).pop_back();
                }

                (edges_.from).pop_back();
                (edges_.to).pop_back();
                return success;
            }
        }

        return true;
    }

    // Remove an edge, this will have to invalidate any EdgeData reference.
    [[maybe_unused]] bool remove_edge(VertexID from, VertexID to)
        requires EdgeMutable<Representation>
    {
        EdgeID res = representation_.remove_edge(from, to);

        if (res == EdgeID::empty)
        {
            return false;
        }

        // If edge was not empty, remove from the list.
        free_edges_.push_back(res);

        edges_.from[res.value] = VertexID::empty;
        edges_.to[res.value] = VertexID::empty;

        return true;
    }

    // Remove and return an edge's data. EdgeData must not be monostate.
    bool try_pop_edge(VertexID from, VertexID to, EdgeData & out)
        requires (!std::is_same_v<EdgeData, std::monostate>
                 && EdgeMutable<Representation>)
    {
        EdgeID res = representation_.remove_edge(from, to);

        if (res == EdgeID::empty)
        {
            return false;
        }

        // If edge was not empty, remove from the list.
        free_edges_.push_back(res);

        // Overwrite with default data.
        out = std::move(edges_.data[res.value]);

        edges_.data[res.value] = {};
        edges_.from[res.value] = VertexID::empty;
        edges_.to[res.value] = VertexID::empty;

        return true;
    }

    void clear()
        noexcept(noexcept(representation_.clear()))
        requires Clearable<Representation>
    {
        representation_.clear();

        // Also clear the stale data in PropertyGraph.

        // TODO: how should we handle this? We have nothing to
        //       set vertices or anything of the sort.
        //
        //       users would also probably be better making a new graph
        //       if we are just wiping everything out.
        // vertices_.clear();

        edges_.clear();
        free_edges_.clear();
    }

    // std::pair overloads.

    // By default, return the EdgeID
    EdgeID find_edge(std::pair<VertexID, VertexID> edge) const
        noexcept(noexcept(find_edge(edge.first, edge.second)))
        requires EdgeQueryable<Representation>
    {
        return find_edge(edge.first, edge.second);
    }

    bool has_edge(std::pair<VertexID, VertexID> edge) const
        noexcept(noexcept(has_edge(edge.first, edge.second)))
        requires EdgeQueryable<Representation>
    {
        return has_edge(edge.first, edge.second);
    }

    template<typename... Args>
    bool add_edge(std::pair<VertexID, VertexID> edge, Args&&... args)
        requires (std::is_same_v<EdgeData, std::monostate>
                  && sizeof...(Args) == 0) ||
                 (!std::is_same_v<EdgeData, std::monostate>
                  && std::constructible_from<EdgeData, Args...>)
                 && EdgeMutable<Representation>
    {
        return add_edge(edge.first, edge.second, std::forward<Args>(args)...);
    }

    [[maybe_unused]] bool remove_edge(std::pair<VertexID, VertexID> edge)
        requires EdgeMutable<Representation>
    {
        return remove_edge(edge.first, edge.second);
    }

    bool try_pop_edge(std::pair<VertexID, VertexID> edge, EdgeData & out)
        requires (!std::is_same_v<EdgeData, std::monostate>)
                 && EdgeMutable<Representation>
    {
        return try_pop_edge(edge.first, edge.second, out);
    }

    // TODO: maybe give overload like std::optional<EdgeData> pop_edge(...)
    //
    // TODO: maybe also provide erase_edge() for explicit immediate overwrites

    //
    // Utility functions
    //

    size_t vertex_count() const noexcept
    {
        return vertices_.size();
    }

    size_t edge_count() const noexcept
    {
        return (edges_.size() - free_edges_.size());
    }

    size_t degree(VertexID v) const
        noexcept(noexcept(representation_.degree(v)))
        requires DegreeQueryable<Representation>
    {
        return representation_.degree(v);
    }

    size_t in_degree(VertexID v) const
        noexcept(noexcept(representation_.in_degree(v)))
        requires (is_directed) && InDegreeQueryable<Representation>
    {
        return representation_.in_degree(v);
    }

    size_t out_degree(VertexID v) const
        noexcept(noexcept(representation_.out_degree(v)))
        requires (is_directed) && OutDegreeQueryable<Representation>
    {
        return representation_.out_degree(v);
    }

    bool has_self_loops() const
        noexcept(noexcept(representation_.has_self_loops()))
    {
        return representation_.has_self_loops();
    }

    bool has_self_loop(VertexID v) const
        noexcept(noexcept(representation_.has_self_loop(v)))
        requires SelfLoopQueryable<Representation>
    {
        return representation_.has_self_loop(v);
    }

    bool is_valid_vertex(VertexID v) const
        noexcept(noexcept(representation_.is_valid_vertex(v)))
        requires SelfLoopQueryable<Representation>
    {
        return representation_.is_valid_vertex(v);
    }

    //
    // Transformations
    //

    // Compute a new graph where the edges are reversed.
    [[nodiscard]] PropertyGraph compute_transpose() const
        requires Transposable<Representation>
    {
        Representation rev = representation_.compute_transpose();

        return PropertyGraph {
            std::move(rev),
            vertices_,
            edges_.reverse_copy(),
            free_edges_
        };
    }

    // TODO: transpose view

private:
    // Internal structure-of-arrays storage.
    struct EdgeArrays
    {
        void clear() noexcept
        {
            from.clear();
            to.clear();

            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                data.clear();
            }
        }

        void resize(size_t N) noexcept
        {
            from.resize(N);
            to.resize(N);

            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                data.resize(N);
            }
        }

        void reserve(size_t N) noexcept
        {
            from.reserve(N);
            to.reserve(N);

            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                data.reserve(N);
            }
        }

        // TODO: debug build asserts that these all line up
        size_t size() const noexcept
        {
            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                return data.size();
            }
            else
            {
                return from.size();
            }
        }

        // For returning a transpose ready copy of the array.
        EdgeArrays reverse_copy() const
        {
            EdgeArrays result;
            result.from = to;
            result.to = from;

            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                result.data = data;
            }

            return result;
        }

        std::vector<VertexID> from;
        std::vector<VertexID> to;

        // Define "std::vector<EdgeData> data" if not monostate.
        std::conditional_t<
            std::is_same_v<EdgeData, std::monostate>,
            std::monostate,
            std::vector<EdgeData>> data;
    };

    // Internal constructor for returning a copy of the graph.
    PropertyGraph(Representation rep,
                  std::vector<VertexData> v,
                  EdgeArrays e,
                  std::vector<EdgeID> free)
    :representation_(std::move(rep)),
    vertices_(std::move(v)),
    edges_(std::move(e)),
    free_edges_(std::move(free))
    {
    }

private:
    Representation representation_;
    std::vector<VertexData> vertices_;

    // stores {from, to, data} entries.
    EdgeArrays edges_;
    // index values into edges_ where we have previously deleted an element.
    std::vector<EdgeID> free_edges_;
};

};
