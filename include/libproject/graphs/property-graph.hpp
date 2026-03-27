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
        requires (!HasData<EdgeData>)
    :from(f),
    to(t)
    {}

    // Construct an EdgeInput from something like
    // {VertexID, VertexID}
    EdgeInput(VertexID f, VertexID t, EdgeData d)
        requires (HasData<EdgeData>)
    :from(f),
    to(t),
    data(std::move(d))
    {}

    // Construct an EdgeInput from something like
    // {{VertexID, VertexID}, EdgeData}
    //
    // Example: {{0, 1}, "edge value"}
    EdgeInput(std::pair<VertexID, VertexID> verts, EdgeData e)
        requires (HasData<EdgeData>)
    :from(verts.first),
    to(verts.second),
    data(std::move(e))
    {
    }

    // Construct an EdgeInput from something like
    // {VertexID, VertexID, EdgeData}
    EdgeInput(std::tuple<VertexID, VertexID, EdgeData> edge)
        requires (HasData<EdgeData>)
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
    static constexpr bool vertices_mutable = VertexMutable<Representation>;
    static constexpr bool edges_mutable = EdgeMutable<Representation>;

    // Check if EdgeData or VertexData is std::monostate
    static constexpr bool has_edge_data = HasData<EdgeData>;
    static constexpr bool has_vertex_data = HasData<VertexData>;

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
            if constexpr (!has_edge_data)
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
    vertices_(vertex_range)
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
            if constexpr (!has_edge_data)
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

    //
    // View functions
    //

    // View over all valid vertex IDs
    auto vertices() const
        requires (vertices_mutable)
    {
        return std::views::iota(size_t(0),
                                vertices_.size())
               | std::views::filter([this](size_t indx){
                    return vertices_.alive(indx);
                })
               | std::views::transform([](size_t indx){
                   return VertexID{indx};
            });
    }

    // View over all vertex IDs
    auto vertices() const
        requires (!vertices_mutable)
    {
        return std::views::iota(size_t(0),
                                vertex_count())
               | std::views::transform([](size_t indx){
                   return VertexID{indx};
            });
    }

    // View over every edge in the graph.
    auto edges() const
        noexcept(noexcept(representation_.edges()))
        requires (GlobalEdgesQueryable<Representation>)
    {
        return representation_.edges()
               | std::views::transform([this](EdgeID id){
                   return EdgeView{edges_.from[id.value],
                                   edges_.to[id.value],
                                   id};
               });
    }

    // View over every edge ID in the graph.
    auto edge_ids() const
        noexcept(noexcept(representation_.edges()))
        requires (GlobalEdgesQueryable<Representation>)
    {
        return representation_.edges();
    }

    // View over neighboring VertexID values.
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

    // View over neighboring VertexID values (outgoing in a digraph).
    auto neighbors(VertexID v) const
        noexcept(noexcept(out_neighbors(v)))
        requires (is_directed) && OutNeighborsQueryable<Representation>
    {
        return out_neighbors(v);
    }

    // View over outgoing VertexID values for each edge (v, u).
    auto out_neighbors(VertexID v) const
        noexcept(noexcept(representation_.out_neighbors(v)))
        requires (is_directed) && OutNeighborsQueryable<Representation>
    {
        return representation_.out_neighbors(v)
                   | std::views::transform([this](EdgeID id){
                       return edges_.to[id.value];
                   });
    }

    // View over incoming VertexID values for each edge (u, v).
    auto in_neighbors(VertexID v) const
        noexcept(noexcept(representation_.in_neighbors(v)))
        requires (is_directed) && InNeighborsQueryable<Representation>
    {
        return representation_.in_neighbors(v)
                   | std::views::transform([this](EdgeID id){
                       return edges_.from[id.value];
                   });
    }

    // View over edges as EdgeViews
    auto incident_edges(VertexID v) const
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

    // View over edges as EdgeViews (outgoing for digraphs)
    auto incident_edges(VertexID v) const
        noexcept(noexcept(out_edges(v)))
        requires (is_directed) && OutNeighborsQueryable<Representation>
    {
        return out_edges(v);
    }

    // View over outgoing edges as EdgeViews
    auto out_edges(VertexID v) const
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

    // View over incoming edges as EdgeViews
    auto in_edges(VertexID v) const
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

    // View over edges as EdgeIDs
    auto incident_edge_ids(VertexID v) const
        noexcept(noexcept(representation_.neighbors(v)))
        requires(!is_directed) && NeighborsQueryable<Representation>
    {
        return representation_.neighbors(v);
    }

    // View over edges as EdgeIDs (outgoing for digraphs)
    auto incident_edge_ids(VertexID v) const
        noexcept(noexcept(out_edge_ids(v)))
        requires(is_directed) && OutNeighborsQueryable<Representation>
    {
        // For a digraph, we want the outgoing edges.
        return out_edge_ids(v);
    }

    // View over outgoing edges as EdgeIDs
    auto out_edge_ids(VertexID v) const
        noexcept(noexcept(representation_.neighbors(v)))
        requires (is_directed) && OutNeighborsQueryable<Representation>
    {
        return representation_.out_neighbors(v);
    }

    // View over incoming edges as EdgeIDs
    auto in_edge_ids(VertexID v) const
        noexcept(noexcept(representation_.in_neighbors(v)))
        requires (is_directed) && InNeighborsQueryable<Representation>
    {
        return representation_.in_neighbors(v);
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
    std::pair<VertexID, VertexID> endpoints(EdgeID e_id) const noexcept
    {
        return {edges_.from[e_id.value], edges_.to[e_id.value]};
    }

    // For returning a reference to the EdgeData of an edge.
    EdgeData & edge_data(EdgeID e_id) noexcept
        requires (has_edge_data)
    {
        return edges_.data[e_id.value];
    }

    // For copying the EdgeData of an edge.
    const EdgeData & edge_data(EdgeID e_id) const noexcept
        requires (has_edge_data)
    {
        return edges_.data[e_id.value];
    }

    VertexData & vertex_data(VertexID v_id) noexcept
        requires (has_vertex_data)
    {
        return vertices_[v_id.value];
    }

    const VertexData & vertex_data(VertexID v_id) const noexcept
        requires (has_vertex_data)
    {
        return vertices_[v_id.value];
    }

    VertexID source(EdgeID e_id) const noexcept
    {
        return edges_.from[e_id.value];
    }

    VertexID target(EdgeID e_id) const noexcept
    {
        return edges_.to[e_id.value];
    }

    bool has_edge(VertexID from, VertexID to) const
        noexcept(noexcept(representation_.has_edge(from, to)))
        requires EdgeQueryable<Representation>
    {
        return representation_.has_edge(from, to);
    }

    // Add an edge to the graph.
    //
    // On success, return the new EdgeID as {new_id, true}
    // On failure, return the old EdgeID as {old_id, false}
    template<typename... Args>
    std::pair<EdgeID, bool> add_edge(VertexID from, VertexID to, Args&&... args)
        requires ((!has_edge_data && sizeof...(Args) == 0) ||
                  (has_edge_data && std::constructible_from<EdgeData, Args...>))
                 && edges_mutable
    {
        EdgeID e_id;

        // If we have deleted an edge in the past, use the old slot.
        if (!free_edges_.empty())
        {
            e_id = free_edges_.back();
            free_edges_.pop_back();

            EdgeID rep_edge = representation_.add_edge(from, to, e_id);

            // If an edge already existed, stop now.
            if (rep_edge != e_id)
            {
                free_edges_.push_back(e_id);
                return {rep_edge, false};
            }

            if constexpr (has_edge_data)
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
            if constexpr (has_edge_data)
            {
                (edges_.data).emplace_back(std::forward<Args>(args)...);
            }

            (edges_.from).push_back(from);
            (edges_.to).push_back(to);

            EdgeID rep_edge = representation_.add_edge(from, to, e_id);

            // If an edge already existed, stop now.
            if (rep_edge != e_id)
            {
                if constexpr (has_edge_data)
                {
                    (edges_.data).pop_back();
                }

                (edges_.from).pop_back();
                (edges_.to).pop_back();
                return {rep_edge, false};
            }
        }

        return {e_id, true};
    }

    // Remove an edge, this will have to invalidate any EdgeData reference.
    bool remove_edge(VertexID from, VertexID to)
        requires edges_mutable
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
    bool try_extract_edge(VertexID from, VertexID to, EdgeData & out)
        requires (has_edge_data && edges_mutable)
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

    // std::pair and EdgeID overloads.

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
    std::pair<EdgeID, bool> add_edge(std::pair<VertexID, VertexID> edge,
                                     Args&&... args)
        requires ((!has_edge_data && sizeof...(Args) == 0) ||
                  (has_edge_data && std::constructible_from<EdgeData, Args...>))
                 && edges_mutable
    {
        return add_edge(edge.first, edge.second, std::forward<Args>(args)...);
    }

    bool remove_edge(std::pair<VertexID, VertexID> edge)
        requires edges_mutable
    {
        return remove_edge(edge.first, edge.second);
    }

    bool try_extract_edge(std::pair<VertexID, VertexID> edge, EdgeData & out)
        requires (has_edge_data && edges_mutable)
    {
        return try_extract_edge(edge.first, edge.second, out);
    }

    // Remove edge using EdgeID
    // TODO <chore>: re-write these sorts of calls to source, target
    bool remove_edge(EdgeID e_id)
        requires edges_mutable
    {
        return remove_edge(edges_.from[e_id.value], edges_.to[e_id.value]);
    }

    // Extract edge using EdgeID
    bool try_extract_edge(EdgeID e_id, EdgeData & out)
        requires (has_edge_data && edges_mutable)
    {
        return try_extract_edge(edges_.from[e_id.value],
                                edges_.to[e_id.value],
                                out);
    }

    // TODO: maybe give overload like std::optional<EdgeData> pop_edge(...)
    //
    // TODO: maybe also provide erase_edge() for explicit immediate overwrites

    //
    // Vertex opertions
    //

    // For adding vertices
    template<typename... Args>
    VertexID add_vertex(Args&&... args)
        requires ((!has_vertex_data && sizeof...(Args) == 0) ||
                  (has_vertex_data
                   && std::constructible_from<VertexData, Args...>))
                 && vertices_mutable
    {
        VertexID v_id;

        // If we have deleted a vertex in the past, use the old slot
        if (!free_vertices_.empty())
        {
            v_id = free_vertices_.back();
            free_vertices_.pop_back();

            bool success = representation_.add_vertex(v_id);

            // Handle failure.
            if (!success)
            {
                free_vertices_.push_back(v_id);
                return VertexID::empty;
            }

            if constexpr (has_vertex_data)
            {
                vertices_[v_id.value] = VertexData{std::forward<Args>(args)...};
            }
            else
            {
                // Really just an increment to our size_t (data is monostate)
                vertices_.emplace_back();
            }

        }
        // We need a new slot otherwise.
        else
        {
            v_id = vertices_.size();

            if constexpr (has_vertex_data)
            {
                (vertices_).emplace_back(std::forward<Args>(args)...);
            }
            else
            {
                // Really just an increment to our size_t (data is monostate)
                vertices_.emplace_back();
            }

            bool success = representation_.add_vertex(v_id);

            // Undo invalid modification
            if (!success)
            {
                (vertices_).pop_back();

                return VertexID::empty;
            }
        }

        return v_id;
    }

    // For removing vertices (lazy delete the VertexData instance).
    void remove_vertex(VertexID v_id)
        requires vertices_mutable
    {
        representation_.remove_vertex(v_id);
        vertices_.set_dead(v_id.value);
        free_vertices_.push_back(v_id);
    }

    void extract_vertex(VertexID v_id, VertexData & out)
        requires (vertices_mutable && has_vertex_data)
    {
        representation_.remove_vertex(v_id);

        // Overwrite the vertex data.
        out = std::move(vertices_[v_id.value]);
        vertices_[v_id.value] = {};

        vertices_.set_dead(v_id.value);

        free_vertices_.push_back(v_id);
    }

    //
    // Graph features
    //

    size_t vertex_count() const noexcept
    {
        if constexpr (vertices_mutable)
        {
            return (vertices_.size() - free_vertices_.size());
        }
        else
        {
            return vertices_.size();
        }
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

    //
    // Utility functions
    //

    void clear()
        noexcept(noexcept(representation_.clear()))
        requires Clearable<Representation>
    {
        representation_.clear();

        // Also clear the stale data in PropertyGraph.
        vertices_.clear();
        edges_.clear();
        free_edges_.clear();
    }

    void clear_edges()
        noexcept(noexcept(representation_.clear()))
        requires Clearable<Representation>
    {
        representation_.clear();

        // Also clear the stale data in PropertyGraph.
        edges_.clear();
        free_edges_.clear();
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

    // TODO: reconsider when we add vertex mutators
    bool valid_vertex(VertexID v) const
        noexcept(noexcept(representation_.is_valid_vertex(v)))
    {
        return representation_.valid_vertex(v);
    }

    bool valid_edge_id(EdgeID e_id) const
    {
        return (e_id < edges_.size());
    }

    void reserve_vertices(size_t N)
    {
        vertices_.reserve(N);
    }

    void reserve_edges(size_t N)
    {
        edges_.reserve(N);
    }

    // TODO <feature>: We really need functions for compressing the
    //                 edges_ and vertices_ members that the user can
    //                 call to shrink the vectors, otherwise we might
    //                 have a case where N slots are stored but M << N
    //                 are actually alive, wasting space and time.
    //
    //                 perhaps garbage_collect() or compress() is a good name?

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

            if constexpr (has_edge_data)
            {
                data.clear();
            }
        }

        void resize(size_t N) noexcept
        {
            from.resize(N);
            to.resize(N);

            if constexpr (has_edge_data)
            {
                data.resize(N);
            }
        }

        void reserve(size_t N)
        {
            from.reserve(N);
            to.reserve(N);

            if constexpr (has_edge_data)
            {
                data.reserve(N);
            }
        }

        // TODO: debug build asserts that these all line up
        size_t size() const noexcept
        {
            if constexpr (has_edge_data)
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

            if constexpr (has_edge_data)
            {
                result.data = data;
            }

            return result;
        }

        std::vector<VertexID> from;
        std::vector<VertexID> to;

        // Define "std::vector<EdgeData> data" if not monostate.
        std::conditional_t<
            has_edge_data,
            std::vector<EdgeData>,
            std::monostate> data;
    };

    struct VertexStorage
    {
    public:
        VertexStorage() = default;

        VertexStorage(size_t N)
            requires (!has_vertex_data)
        :count_(N)
        {
            if constexpr (vertices_mutable)
            {
                alive_vertices_.resize(N, 1);
            }
        }

        VertexStorage(size_t N)
            requires (has_vertex_data)
        :data_(N)
        {
            if constexpr (vertices_mutable)
            {
                alive_vertices_.resize(N, 1);
            }
        }

        template <std::ranges::sized_range VertexRange>
            requires (has_vertex_data)
        VertexStorage(VertexRange v_range)
        :data_(std::ranges::begin(v_range), std::ranges::end(v_range))
        {
            if constexpr (vertices_mutable)
            {
                alive_vertices_.resize(std::ranges::size(v_range), 1);
            }
        }

        size_t size() const noexcept
        {
            if constexpr (has_vertex_data)
            {
                return data_.size();
            }
            else
            {
                return count_;
            }
        }

        void reserve(size_t N)
        {
            if constexpr (has_vertex_data)
            {
                data_.reserve(N);
            }

            if constexpr (vertices_mutable)
            {
                alive_vertices_.reserve(N);
            }
        }

        void clear() noexcept
        {
            if constexpr (has_vertex_data)
            {
                data_.clear();
            }
            else
            {
                count_ = 0;
            }

            if constexpr (vertices_mutable)
            {
                alive_vertices_.clear();
            }
        }

        void pop_back()
        {
            if constexpr (has_vertex_data)
            {
                data_.pop_back();
            }
            else
            {
                count_--;
            }

            if constexpr (vertices_mutable)
            {
                alive_vertices_.pop_back();
            }
        }

        void emplace_back()
        {
            if constexpr (has_vertex_data)
            {
                data_.emplace_back();
            }
            else
            {
                count_++;
            }

            if constexpr (vertices_mutable)
            {
                alive_vertices_.emplace_back(1);
            }
        }

        template <typename... Args>
        void emplace_back(Args&&... args)
            requires (has_vertex_data
                      && std::constructible_from<VertexData, Args...>)
        {
            data_.emplace_back(std::forward<Args>(args)...);

            if constexpr (vertices_mutable)
            {
                alive_vertices_.emplace_back(1);
            }
        }

        VertexData & operator[](size_t i)
            requires (has_vertex_data)
        {
            return data_[i];
        }

        bool alive(VertexID v_id) const
            requires (vertices_mutable)
        {
            return alive_vertices_[v_id.value];
        }

        void set_alive(VertexID v_id)
            requires (vertices_mutable)
        {
            alive_vertices_[v_id.value] = 1;
        }

        void set_dead(VertexID v_id)
            requires (vertices_mutable)
        {
            alive_vertices_[v_id.value] = 0;
        }

    private:
        std::conditional_t<
            has_vertex_data,
            std::vector<VertexData>,
            std::monostate> data_;

        std::conditional_t<
            vertices_mutable,
            std::vector<uint8_t>,
            std::monostate> alive_vertices_;

        std::conditional_t<
            has_vertex_data,
            std::monostate,
            std::size_t> count_;
    };

    // Internal constructor for returning a copy of the graph.
    PropertyGraph(Representation rep,
                  VertexStorage v,
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

    VertexStorage vertices_;
    EdgeArrays edges_;

    // index values into vertices_ where we have previously deleted an element.
    std::conditional_t<
            vertices_mutable,
            std::vector<VertexID>,
            std::monostate> free_vertices_;

    // index values into edges_ where we have previously deleted an element.
    std::conditional_t<
            edges_mutable,
            std::vector<EdgeID>,
            std::monostate> free_edges_;
};

};
