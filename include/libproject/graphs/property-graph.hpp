#pragma once

#include <variant>
#include <numeric>

#include <libproject/internal/libproject-assert.hpp>
#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-inputs.hpp>
#include <libproject/graphs/representation-concepts.hpp>
#include <libproject/graphs/transpose-view.hpp>

namespace libproject::graphs
{

// TODO <optimization>: We can add a new template parameter
//                      "bool mutable = true" so users can decide
//                      to override the Representation being mutable
//                      without forcing regular users to add a new
//                      template argument to their declaration
//
//                      simply take && of vertices/edges mutable constexpr
//
//                      we can also use some enum MutabilityPolicy as well

template <GraphRepresentation Representation,
          typename EdgeData = std::monostate,
          typename VertexData = std::monostate>
class PropertyGraph
{
public:
    // Map from old ID values to new values after a compression occurs.
    struct CompressionMap
    {
        std::vector<VertexID> old_to_new_vertex;
        std::vector<EdgeID> old_to_new_edge;
    };

public:
    using EdgeDataType = EdgeData;
    using VertexDataType = VertexData;

    static constexpr bool directed = Representation::directed;
    static constexpr bool vertices_mutable = VertexMutable<Representation>;
    static constexpr bool edges_mutable = EdgeMutable<Representation>;

    // Check if EdgeData or VertexData is std::monostate
    static constexpr bool has_edge_data = HasData<EdgeData>;
    static constexpr bool has_vertex_data = HasData<VertexData>;

    enum class EdgeInsertPolicy
    {
        Unchecked,
        Unique,
        PushBack
    };

    // For simplification of pair overloads.
    using EdgePair = std::pair<VertexID, VertexID>;

    // TODO <feature>: should we maybe have representations
    //                 expose "is_multigraph" and keep this
    //                 as a generic container?

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
    template <std::ranges::input_range InputEdgeRange>
    PropertyGraph(size_t num_vertices, InputEdgeRange && edge_range)
        requires (EdgeInputRange<InputEdgeRange, EdgeData> &&
                 PropertyGraphConstructible<Representation> &&
                 !EdgeViewRangeConstructible<
                    Representation,
                    decltype(make_edge_view_range<EdgeData>
                             (std::declval<InputEdgeRange&&>()))>)
    :representation_(num_vertices),
    vertices_(num_vertices)
    {
        // First, lets reserve.
        if constexpr (std::ranges::sized_range<InputEdgeRange>)
        {
            size_t expected_edges = std::ranges::size(edge_range);
            edges_.reserve(expected_edges);
        }

        for (auto && item : edge_range)
        {
            if constexpr (!has_edge_data)
            {
                const auto & [from, to] = item;

                libproject_assert(from.value < vertices_.size()
                                  && to.value < vertices_.size(),
                                  "PropertyGraph::PropertyGraph(num_vertices, "
                                  "edge_range): edge references "
                                  "invalid vertex; edge = ("
                                  + std::to_string(from.value)
                                  + " "
                                  + std::to_string(to.value)
                                  + "), vertex_count = "
                                  + std::to_string(vertices_.size()));

                add_edge(from, to);
            }
            else
            {
                auto && [from, to, data] = item;

                libproject_assert(from.value < vertices_.size()
                                  && to.value < vertices_.size(),
                                  "PropertyGraph::PropertyGraph(num_vertices, "
                                  "edge_range): edge references "
                                  "invalid vertex; edge = ("
                                  + std::to_string(from.value)
                                  + " "
                                  + std::to_string(to.value)
                                  + "), vertex_count = "
                                  + std::to_string(vertices_.size()));

                add_edge(from, to, std::forward<decltype(data)>(data));
            }
        }
    }

    // Cosntruct a graph from a list of verices and a list of edges.
    template <std::ranges::sized_range VertexRange,
              std::ranges::input_range InputEdgeRange>
    PropertyGraph(VertexRange && vertex_range, InputEdgeRange && edge_range)
        requires (PropertyGraphConstructible<Representation> &&
                 EdgeInputRange<InputEdgeRange, EdgeData> &&
                 !EdgeViewRangeConstructible<
                    Representation,
                    decltype(make_edge_view_range<EdgeData>
                             (std::declval<InputEdgeRange&&>()))>)
    :representation_(std::ranges::size(vertex_range)),
    vertices_(vertex_range)
    {
        // First, lets reserve.
        if constexpr (std::ranges::sized_range<InputEdgeRange>)
        {
            size_t expected_edges = std::ranges::size(edge_range);
            edges_.reserve(expected_edges);
        }

        for (auto && item : edge_range)
        {
            if constexpr (!has_edge_data)
            {
                const auto & [from, to] = item;

                libproject_assert(from.value < vertices_.size()
                                  && to.value < vertices_.size(),
                                  "PropertyGraph::PropertyGraph(vertex_range, "
                                  "edge_range): edge references "
                                  "invalid vertex; edge = ("
                                  + std::to_string(from.value)
                                  + " "
                                  + std::to_string(to.value)
                                  + "), vertex_count = "
                                  + std::to_string(vertices_.size()));

                add_edge(from, to);
            }
            else
            {
                auto && [from, to, data] = item;

                libproject_assert(from.value < vertices_.size()
                                  && to.value < vertices_.size(),
                                  "PropertyGraph::PropertyGraph(vertex_range, "
                                  "edge_range): edge references "
                                  "invalid vertex; edge = ("
                                  + std::to_string(from.value)
                                  + " "
                                  + std::to_string(to.value)
                                  + "), vertex_count = "
                                  + std::to_string(vertices_.size()));

                add_edge(from, to, std::forward<decltype(data)>(data));
            }
        }
    }

    //
    // Constructor for representations that expose a range constructor
    //

    template <typename ER>
    PropertyGraph(size_t num_vertices, ER && edge_range)
        requires EdgeInputRange<ER, EdgeData> &&
                 EdgeViewRangeConstructible<
                    Representation,
                    decltype(make_edge_view_range<EdgeData>
                             (std::declval<ER&&>()))>
    :representation_(num_vertices,
                     make_edge_view_range<EdgeData>(
                         std::forward<ER>(edge_range))),
    vertices_(num_vertices)
    {
        if constexpr (std::ranges::sized_range<ER>)
        {
            size_t expected_edges = std::ranges::size(edge_range);
            edges_.reserve(expected_edges);
        }

        for (auto && item : edge_range)
        {
            if constexpr (!has_edge_data)
            {
                const auto & [from, to] = item;

                libproject_assert(from.value < vertices_.size()
                                  && to.value < vertices_.size(),
                                  "PropertyGraph::PropertyGraph(num_vertices, "
                                  "edge_range): edge references "
                                  "invalid vertex; edge = ("
                                  + std::to_string(from.value)
                                  + " "
                                  + std::to_string(to.value)
                                  + "), vertex_count = "
                                  + std::to_string(vertices_.size()));

                push_back_edge(from, to);
            }
            else
            {
                auto && [from, to, data] = item;

                libproject_assert(from.value < vertices_.size()
                                  && to.value < vertices_.size(),
                                  "PropertyGraph::PropertyGraph(num_vertices, "
                                  "edge_range): edge references "
                                  "invalid vertex; edge = ("
                                  + std::to_string(from.value)
                                  + " "
                                  + std::to_string(to.value)
                                  + "), vertex_count = "
                                  + std::to_string(vertices_.size()));

                push_back_edge(from, to, std::forward<decltype(data)>(data));
            }
        }
    }

    template <std::ranges::sized_range VertexRange,
              typename ER>
    PropertyGraph(VertexRange vertex_range,
                  ER && edge_range)
        requires EdgeInputRange<ER, EdgeData> &&
                 EdgeViewRangeConstructible<
                    Representation,
                    decltype(make_edge_view_range<EdgeData>
                             (std::declval<ER&&>()))>
    :representation_(std::ranges::size(vertex_range),
                     make_edge_view_range<EdgeData>(
                         std::forward<ER>(edge_range))),
    vertices_(vertex_range)
    {
        if constexpr (std::ranges::sized_range<ER>)
        {
            size_t expected_edges = std::ranges::size(edge_range);
            edges_.reserve(expected_edges);
        }

        for (auto && item : edge_range)
        {
            if constexpr (!has_edge_data)
            {
                const auto & [from, to] = item;

                libproject_assert(from.value < vertices_.size()
                                  && to.value < vertices_.size(),
                                  "PropertyGraph::PropertyGraph(vertex_range, "
                                  "edge_range): edge references "
                                  "invalid vertex; edge = ("
                                  + std::to_string(from.value)
                                  + " "
                                  + std::to_string(to.value)
                                  + "), vertex_count = "
                                  + std::to_string(vertices_.size()));

                push_back_edge(from, to);
            }
            else
            {
                auto && [from, to, data] = item;

                libproject_assert(from.value < vertices_.size()
                                  && to.value < vertices_.size(),
                                  "PropertyGraph::PropertyGraph(vertex_range, "
                                  "edge_range): edge references "
                                  "invalid vertex; edge = ("
                                  + std::to_string(from.value)
                                  + " "
                                  + std::to_string(to.value)
                                  + "), vertex_count = "
                                  + std::to_string(vertices_.size()));

                push_back_edge(from, to, std::forward<decltype(data)>(data));
            }
        }
    }

    //
    // View functions
    //

    // View over all valid vertex IDs
    decltype(auto) vertices() const
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
    decltype(auto) vertices() const
        requires (!vertices_mutable)
    {
        return std::views::iota(size_t(0),
                                vertex_count())
               | std::views::transform([](size_t indx){
                   return VertexID{indx};
            });
    }

    // View over every edge in the graph as an EdgeView.
    decltype(auto) edges() const
    {
        return edge_ids()
               | std::views::transform([this](EdgeID id){
                   return EdgeView{source(id),
                                   target(id),
                                   id};
            });
    }

    // View over every edge ID in the graph.
    decltype(auto) edge_ids() const
    {
        return std::views::iota(size_t{0}, edges_.size())
               | std::views::filter([this](size_t id){
                   return edges_.from[id] != VertexID::empty;
               })
               | std::views::transform([](size_t id){
                   return EdgeID{static_cast<EdgeID>(id)};
               });
    }

    //
    // Views for getting neighboring VertexIDs.
    //

    // View over neighboring VertexID values.
    decltype(auto) neighbors(VertexID v) const
        noexcept(noexcept(representation_.neighbors(v)))
        requires (!directed) && NeighborsQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::neighbors(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.neighbors(v);
    }

    // View over neighboring VertexID values (outgoing in a digraph).
    decltype(auto) neighbors(VertexID v) const
        noexcept(noexcept(out_neighbors(v)))
        requires (directed) &&
                 OutNeighborsQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::neighbors(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return out_neighbors(v);
    }

    // View over outgoing VertexID values for each edge (v, u).
    decltype(auto) out_neighbors(VertexID v) const
        noexcept(noexcept(representation_.out_neighbors(v)))
        requires (directed) &&
                 OutNeighborsQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::out_neighbors(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.out_neighbors(v);
    }

    // View over incoming VertexID values for each edge (u, v).
    decltype(auto) in_neighbors(VertexID v) const
        noexcept(noexcept(representation_.in_neighbors(v)))
        requires (directed) && InNeighborsQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::in_neighbors(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.in_neighbors(v);
    }

    //
    // Views for getting neighboring IncidentEdges.
    //

    // View over edges as IncidentEdges.
    decltype(auto) incident_edges(VertexID v) const
        noexcept(noexcept(representation_.incident_edges(v)))
        requires (!directed) &&
                 IncidentEdgesQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::incident_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.incident_edges(v);
    }

    // View over edges as IncidentEdges, only used if the
    // representation doesn't expose incident_edges(v)
    //
    // This version is slower because we will have more
    // cache misses each time in a large graph.
    decltype(auto) incident_edges(VertexID v) const
        noexcept(
            noexcept(representation_.incident_edge_ids(v)) &&
            noexcept(IncidentEdge{target(EdgeID{}), EdgeID{}})
        )
        requires (!directed &&
                 IncidentEdgeIDsQueryable<Representation> &&
                 !IncidentEdgesQueryable<Representation>)
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::incident_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.incident_edge_ids(v)
                | std::views::transform([this](EdgeID id){
                    return IncidentEdge{target(id),
                                        id};
                });
    }

    // View over edges as an IncidentEdge (outgoing for digraphs)
    decltype(auto) incident_edges(VertexID v) const
        noexcept(noexcept(out_edges(v)))
        requires (directed) &&
                 (OutEdgesQueryable<Representation>
                  || OutEdgeIDsQueryable<Representation>)
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::incident_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return out_edges(v);
    }

    // View over outgoing edges as IncidentEdges.
    decltype(auto) out_edges(VertexID v) const
        noexcept(noexcept(representation_.out_edges(v)))
        requires (directed) &&
                 OutEdgesQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::out_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.out_edges(v);
    }

    // View over outgoing edges as IncidentEdges.
    //
    // Only used if representation doesn't expose an
    // out_edges method.
    decltype(auto) out_edges(VertexID v) const
        noexcept(
            noexcept(representation_.out_edge_ids(v)) &&
            noexcept(IncidentEdge{target(EdgeID{}), EdgeID{}})
        )
        requires (directed &&
                 !OutEdgesQueryable<Representation> &&
                 OutEdgeIDsQueryable<Representation>)
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::out_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.out_edge_ids(v)
               | std::views::transform([this](EdgeID id){
                    return IncidentEdge{target(id),
                                        id};
               });
    }

    // View over incoming edges as IncidentEdges
    decltype(auto) in_edges(VertexID v) const
        noexcept(noexcept(representation_.in_edges(v)))
        requires (directed) && InEdgesQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::in_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.in_edges(v);
    }

    // View over incoming edges as IncidentEdges.
    //
    // Only used if representation doesn't expose an
    // in_edges method.
    decltype(auto) in_edges(VertexID v) const
        noexcept(
            noexcept(representation_.in_edge_ids(v)) &&
            noexcept(IncidentEdge{source(EdgeID{}), EdgeID{}})
        )
        requires (directed &&
                 !InEdgesQueryable<Representation> &&
                 InEdgeIDsQueryable<Representation>)
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::in_edges(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.in_edge_ids(v)
               | std::views::transform([this](EdgeID id){
                    return IncidentEdge{source(id), id};
               });
    }

    //
    // Views for getting neighboring EdgeIDs.
    //

    // View over edges as EdgeIDs
    decltype(auto) incident_edge_ids(VertexID v) const
        noexcept(noexcept(representation_.incident_edge_ids(v)))
        requires (!directed &&
                 IncidentEdgeIDsQueryable<Representation>)
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::incident_edge_ids(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.incident_edge_ids(v);
    }

    // View over edges as EdgeIDs (outgoing for digraphs)
    decltype(auto) incident_edge_ids(VertexID v) const
        noexcept(noexcept(out_edge_ids(v)))
        requires(directed) && OutEdgeIDsQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::incident_edge_ids(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        // For a digraph, we want the outgoing edges.
        return out_edge_ids(v);
    }

    // View over outgoing edges as EdgeIDs
    decltype(auto) out_edge_ids(VertexID v) const
        noexcept(noexcept(representation_.out_edge_ids(v)))
        requires (directed) && OutEdgeIDsQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::out_edge_ids(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.out_edge_ids(v);
    }

    // View over incoming edges as EdgeIDs
    decltype(auto) in_edge_ids(VertexID v) const
        noexcept(noexcept(representation_.in_edge_ids(v)))
        requires (directed) && InEdgeIDsQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::in_edge_ids(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.in_edge_ids(v);
    }

    //
    // Edge operations
    //

    // By default, return the EdgeID
    EdgeID find_edge(VertexID from, VertexID to) const
        noexcept(noexcept(representation_.find_edge(from, to)))
        requires EdgeQueryable<Representation>
    {
        libproject_assert(valid_vertex(from),
                          "PropertyGraph::find_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "PropertyGraph::find_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

        return representation_.find_edge(from, to);
    }

    // Given an edge ID, find the vertices such that (u, v) has this ID.
    EdgePair endpoints(EdgeID e_id) const noexcept
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::endpoints(e_id): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        libproject_assert(valid_vertex(source(e_id))
                          && valid_vertex(target(e_id)),
                          "PropertyGraph::endpoints(e_id): "
                          "internal library error: valid_edge(e_id) "
                          "returned true but endpoints are invalid; "
                          "e_id = " + std::to_string(e_id.value) +
                          ", source = " + std::to_string(source(e_id).value) +
                          ", target = " + std::to_string(target(e_id).value));

        return {source(e_id), target(e_id)};
    }

    // For returning a reference to the EdgeData of an edge.
    EdgeData & edge_data(EdgeID e_id) noexcept
        requires (has_edge_data)
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::edge_data(e_id): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        return edges_.data[e_id.value];
    }

    // For copying the EdgeData of an edge.
    const EdgeData & edge_data(EdgeID e_id) const noexcept
        requires (has_edge_data)
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::edge_data(e_id): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        return edges_.data[e_id.value];
    }

    VertexData & vertex_data(VertexID v) noexcept
        requires (has_vertex_data)
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::vertex_data(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return vertices_[v.value];
    }

    const VertexData & vertex_data(VertexID v) const noexcept
        requires (has_vertex_data)
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::vertex_data(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return vertices_[v.value];
    }

    VertexID source(EdgeID e_id) const noexcept
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::source(e_id): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        libproject_assert(valid_vertex(edges_.from[e_id.value]),
                          "PropertyGraph::source(e_id): "
                          "internal library error: valid_edge(e_id) "
                          "returned true but source vertex is invalid; "
                          "e_id = " + std::to_string(e_id.value) +
                          ", vertex = "
                          + std::to_string(edges_.from[e_id.value].value));

        return edges_.from[e_id.value];
    }

    VertexID target(EdgeID e_id) const noexcept
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::target(e_id): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        libproject_assert(valid_vertex(edges_.to[e_id.value]),
                          "PropertyGraph::target(e_id): "
                          "internal library error: valid_edge(e_id) "
                          "returned true but target vertex is invalid; "
                          "e_id = " + std::to_string(e_id.value) +
                          ", vertex = "
                          + std::to_string(edges_.to[e_id.value].value));

        return edges_.to[e_id.value];
    }

    bool has_edge(VertexID from, VertexID to) const
        noexcept(noexcept(representation_.has_edge(from, to)))
        requires EdgeQueryable<Representation>
    {
        libproject_assert(valid_vertex(from),
                          "PropertyGraph::has_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "PropertyGraph::has_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

        return representation_.has_edge(from, to);
    }

    // Add an edge to the graph.
    //
    // On success, return the new EdgeID as {new_id, true}
    // On failure, return the old EdgeID as {old_id, false}
    template<EdgeInsertPolicy Policy, typename... Args>
    std::pair<EdgeID, bool> add_edge_impl(VertexID from, VertexID to, Args&&... args)
        requires ((!has_edge_data && sizeof...(Args) == 0) ||
                  (has_edge_data && std::constructible_from<EdgeData, Args...>))
                 && edges_mutable
    {
        libproject_assert(valid_vertex(from),
                          "PropertyGraph::add_edge_impl(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "PropertyGraph::add_edge_impl(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

        EdgeID e_id;
        EdgeID rep_edge;

        // If we have deleted an edge in the past, use the old slot.
        if (!free_edges_.empty())
        {
            e_id = free_edges_.back();
            free_edges_.pop_back();

            if constexpr (Policy == EdgeInsertPolicy::Unique)
            {
                // If we expose add_edge_unique then use this.
                if constexpr (requires {representation_
                                            .add_edge_unique(from, to, e_id);})
                {
                    rep_edge = representation_.add_edge_unique(from,
                                                               to,
                                                               e_id);
                }
                // If the representation doesn't expose a uniqueness
                // check, then we can do it ourself, but it might be slower.
                else
                {
                    rep_edge = representation_.find_edge(from, to);

                    if (rep_edge == EdgeID::empty)
                    {
                        rep_edge = representation_.add_edge(from, to, e_id);
                    }
                }
            }
            // This should never happen, we only use PushBack internally
            // for range based constructors where representation has .build().
            else if constexpr (Policy == EdgeInsertPolicy::PushBack)
            {
                libproject_assert(false,
                                  "PropertyGraph::push_back_edge(from, to): "
                                  "internal library error: tried to use "
                                  "free edge "
                                  + std::to_string(e_id.value)
                                  + " but free_edges_ should be "
                                  "empty during construction.");
            }
            else
            {
                rep_edge = representation_.add_edge(from, to, e_id);
            }

            // Do nothing if EdgeInsertPolicy::PushBack

            // If an edge already existed, stop now.
            if (rep_edge != e_id)
            {
                free_edges_.push_back(e_id);
                return {rep_edge, false};
            }

            libproject_assert(e_id.value < edges_.size(),
                              "PropertyGraph::add_edge(from, to): "
                              "internal library error: free edges list "
                              "returned EdgeID out of range; "
                              "edge_id = " + std::to_string(e_id.value)
                              + " |E| = " + std::to_string(edges_.size()));

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
            // TODO <feature>: function the user can call to check that
            //                 each array is of equal size so they can be fixed.
            if constexpr (has_edge_data)
            {
                (edges_.data).emplace_back(std::forward<Args>(args)...);
            }

            (edges_.from).push_back(from);
            (edges_.to).push_back(to);

            // Representation insertion based on policy.
            if constexpr (Policy == EdgeInsertPolicy::Unique)
            {
                // If we expose add_edge_unique then use this.
                if constexpr (requires {representation_
                                            .add_edge_unique(from, to, e_id);})
                {
                    rep_edge = representation_.add_edge_unique(from,
                                                               to,
                                                               e_id);
                }
                // If the representation doesn't expose a uniqueness
                // check, then we can do it ourself, but it might be slower.
                else
                {
                    rep_edge = representation_.find_edge(from, to);

                    if (rep_edge == EdgeID::empty)
                    {
                        rep_edge = representation_.add_edge(from, to, e_id);
                    }
                }
            }
            else if constexpr (Policy == EdgeInsertPolicy::PushBack)
            {
                // Stop early, we have no failure case.
                return {e_id, true};
            }
            else
            {
                rep_edge = representation_.add_edge(from, to, e_id);
            }

            // If an edge already existed, stop now.
            if (rep_edge != e_id)
            {
                libproject_assert(valid_edge(e_id),
                                  "PropertyGraph::add_edge(from, to): "
                                  "internal library error: representation "
                                  "returned an invalid EdgeID; "
                                  "edge_id = " + std::to_string(e_id.value));

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

    template<typename... Args>
    auto add_edge(VertexID from, VertexID to, Args&&... args)
    {
        return add_edge_impl<EdgeInsertPolicy::Unchecked>(
            from, to, std::forward<Args>(args)...);
    }

    // Remove an edge, this will have to invalidate any EdgeData reference.
    bool remove_edge(VertexID from, VertexID to)
        requires edges_mutable
    {
        libproject_assert(valid_vertex(from),
                          "PropertyGraph::remove_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "PropertyGraph::remove_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

        EdgeID res = representation_.remove_edge(from, to);

        if (res == EdgeID::empty)
        {
            return false;
        }

        libproject_assert(valid_edge(res),
                          "PropertyGraph::remove_edge(from, to): "
                          "internal library error: representation "
                          "returned an invalid EdgeID; "
                          "edge_id = " + std::to_string(res.value));

        // If edge was not empty, remove from the list.
        free_edges_.push_back(res);

        edges_.from[res.value] = VertexID::empty;
        edges_.to[res.value] = VertexID::empty;

        return true;
    }

    // Remove an edge and overwrite instead of lazy deletion.
    bool erase_edge(VertexID from, VertexID to)
        requires edges_mutable
    {
        libproject_assert(valid_vertex(from),
                          "PropertyGraph::erase_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "PropertyGraph::erase_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

        EdgeID res = representation_.remove_edge(from, to);

        if (res == EdgeID::empty)
        {
            return false;
        }

        libproject_assert(valid_edge(res),
                          "PropertyGraph::erase_edge(from, to): "
                          "internal library error: representation "
                          "returned an invalid EdgeID; "
                          "edge_id = " + std::to_string(res.value));

        // If edge was not empty, remove from the list.
        free_edges_.push_back(res);

        if constexpr (has_edge_data)
        {
            auto temp = std::move(edges_.data[res.value]);
        }

        edges_.from[res.value] = VertexID::empty;
        edges_.to[res.value] = VertexID::empty;

        return true;
    }

    // Remove an edge and move its data.
    //
    // Returns false if the edge doesn't exist.
    bool extract_edge(VertexID from, VertexID to, EdgeData & out)
        requires (has_edge_data && edges_mutable)
    {
        libproject_assert(valid_vertex(from),
                          "PropertyGraph::extract_edge(from, to, out): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "PropertyGraph::extract_edge(from, to, out): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

        EdgeID res = representation_.remove_edge(from, to);

        if (res == EdgeID::empty)
        {
            return false;
        }

        libproject_assert(valid_edge(res),
                          "PropertyGraph::extract_edge(from, to, out): "
                          "internal library error: representation "
                          "returned an invalid EdgeID; "
                          "edge_id = " + std::to_string(res.value));

        // If edge was not empty, remove from the list.
        free_edges_.push_back(res);

        out = std::move(edges_.data[res.value]);

        edges_.from[res.value] = VertexID::empty;
        edges_.to[res.value] = VertexID::empty;

        return true;
    }

    // Remove an edge and return the edge's data if it exists.
    std::optional<EdgeData> extract_edge(VertexID from, VertexID to)
        requires (has_edge_data && edges_mutable)
    {
        libproject_assert(valid_vertex(from),
                          "PropertyGraph::extract_edge(from, to): "
                          "invalid VertexID; from = "
                          + std::to_string(from.value));

        libproject_assert(valid_vertex(to),
                          "PropertyGraph::extract_edge(from, to): "
                          "invalid VertexID; to = "
                          + std::to_string(to.value));

        EdgeID res = representation_.remove_edge(from, to);

        if (res == EdgeID::empty)
        {
            return {};
        }

        libproject_assert(valid_edge(res),
                          "PropertyGraph::extract_edge(from, to): "
                          "internal library error: representation "
                          "returned an invalid EdgeID; "
                          "edge_id = " + std::to_string(res.value));

        // If edge was not empty, remove from the list.
        free_edges_.push_back(res);

        auto data = std::move(edge_data(res));

        edges_.from[res.value] = VertexID::empty;
        edges_.to[res.value] = VertexID::empty;

        return data;
    }

    //
    // std::pair and EdgeID overloads.
    //

    EdgeID find_edge(EdgePair edge) const
        noexcept(noexcept(find_edge(edge.first, edge.second)))
        requires EdgeQueryable<Representation>
    {
        return find_edge(edge.first, edge.second);
    }

    bool has_edge(EdgePair edge) const
        noexcept(noexcept(has_edge(edge.first, edge.second)))
        requires EdgeQueryable<Representation>
    {
        return has_edge(edge.first, edge.second);
    }

    template<typename... Args>
    std::pair<EdgeID, bool> add_edge(EdgePair edge,
                                     Args&&... args)
        requires ((!has_edge_data && sizeof...(Args) == 0) ||
                  (has_edge_data && std::constructible_from<EdgeData, Args...>))
                 && edges_mutable
    {
        return add_edge(edge.first, edge.second, std::forward<Args>(args)...);
    }

    bool remove_edge(EdgePair edge)
        requires edges_mutable
    {
        return remove_edge(edge.first, edge.second);
    }

    bool erase_edge(EdgePair edge)
        requires edges_mutable
    {
        return erase_edge(edge.first, edge.second);
    }

    bool extract_edge(EdgePair edge, EdgeData & out)
        requires (has_edge_data && edges_mutable)
    {
        return extract_edge(edge.first, edge.second, out);
    }

    std::optional<EdgeData> extract_edge(EdgePair edge)
        requires (has_edge_data && edges_mutable)
    {
        return extract_edge(edge.first, edge.second);
    }

    // Remove edge using EdgeID
    bool remove_edge(EdgeID e_id)
        requires edges_mutable
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::remove_edge(e_id): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        return remove_edge(source(e_id), target(e_id));
    }

    bool erase_edge(EdgeID e_id)
        requires edges_mutable
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::erase_edge(e_id): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        return erase_edge(source(e_id), target(e_id));
    }

    // Extract edge using EdgeID
    bool extract_edge(EdgeID e_id, EdgeData & out)
        requires (has_edge_data && edges_mutable)
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::extract_edge(e_id, out): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        return extract_edge(source(e_id), target(e_id), out);
    }

    std::optional<EdgeData> extract_edge(EdgeID e_id)
        requires (has_edge_data && edges_mutable)
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::extract_edge(e_id): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        return extract_edge(source(e_id), target(e_id));
    }

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
                // Increment to our size_t (data is monostate)
                vertices_.increment_counter();
            }

            // Set vertex as alive.
            if constexpr (vertices_mutable)
            {
                vertices_.set_alive(v_id);
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
    void remove_vertex(VertexID v)
        requires vertices_mutable
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::remove_vertex(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        // Remove the vertex, with a callback to remove the edge in the
        // PropertyGraph whenever we remove one in the representation.
        representation_.remove_vertex(v, [this](EdgeID e_id){
            this->remove_edge(e_id);
        });

        vertices_.set_dead(v.value);
        free_vertices_.push_back(v);
    }

    void extract_vertex(VertexID v, VertexData & out)
        requires (vertices_mutable && has_vertex_data)
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::extract_vertex(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        // Overwrite the vertex data.
        out = std::move(vertices_[v.value]);

        representation_.remove_vertex(v, [this](EdgeID e_id){
            this->remove_edge(e_id);
        });

        vertices_.set_dead(v.value);

        free_vertices_.push_back(v);
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
        if constexpr (edges_mutable)
        {
            return (edges_.size() - free_edges_.size());
        }

        return edges_.size();
    }

    size_t degree(VertexID v) const
        noexcept(noexcept(representation_.degree(v)))
        requires DegreeQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::degree(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.degree(v);
    }

    size_t in_degree(VertexID v) const
        noexcept(noexcept(representation_.in_degree(v)))
        requires (directed) && InDegreeQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::in_degree(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.in_degree(v);
    }

    size_t out_degree(VertexID v) const
        noexcept(noexcept(representation_.out_degree(v)))
        requires (directed) && OutDegreeQueryable<Representation>
    {
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::out_degree(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.out_degree(v);
    }

    //
    // Utility functions
    //

    EdgeView edge_view(EdgeID e_id) const
        noexcept(noexcept(EdgeView{source(e_id), target(e_id), e_id}))
    {
        libproject_assert(valid_edge(e_id),
                          "PropertyGraph::edge_view(e_id): "
                          "invalid EdgeID; e_id = "
                          + std::to_string(e_id.value));

        return EdgeView{source(e_id), target(e_id), e_id};
    }

    // Return const or non-const VertexData based on lvalue
    VertexData & operator[](VertexID v) noexcept
        requires (has_vertex_data)
    {
        return vertex_data(v);
    }

    const VertexData & operator[](VertexID v) const noexcept
        requires (has_vertex_data)
    {
        return vertex_data(v);
    }

    // Return const or non-const EdgeData based on lvalue
    EdgeData & operator[](EdgeID e) noexcept
        requires (has_edge_data)
    {
        return edge_data(e);
    }

    const EdgeData & operator[](EdgeID e) const noexcept
        requires (has_edge_data)
    {
        return edge_data(e);
    }

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

    void clear_vertices()
        noexcept(noexcept(vertices_.clear()))
    {
        vertices_.clear();
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
        libproject_assert(valid_vertex(v),
                          "PropertyGraph::has_self_loop(v): "
                          "invalid VertexID; v = "
                          + std::to_string(v.value));

        return representation_.has_self_loop(v);
    }

    bool valid_vertex(VertexID v) const
        noexcept(noexcept(representation_.valid_vertex(v)))
    {
        // If vertices are able to be mutated, we must validate that
        // the vertex is alive.
        if constexpr (vertices_mutable)
        {
            return representation_.valid_vertex(v)
                   && (v.value < vertices_.size())
                   && vertices_.alive(v);
        }

        return representation_.valid_vertex(v)
               && v.value < vertices_.size();
    }

    bool valid_edge(EdgeID e_id) const noexcept
    {
        return (e_id < edges_.size()
                && edges_.from[e_id.value] != VertexID::empty);
    }

    void reserve_vertices(size_t N)
    {
        vertices_.reserve(N);
    }

    void reserve_edges(size_t N)
    {
        edges_.reserve(N);
    }

    // This function compresses the graph to improve
    // performance after many remove operations.
    CompressionMap compress()
        requires (vertices_mutable && edges_mutable)
    {
        CompressionMap map;
        auto & vertex_map = map.old_to_new_vertex;
        auto & edge_map = map.old_to_new_edge;

        // Set the size (current in use range)
        vertex_map.resize(vertices_.size(), VertexID::empty);
        edge_map.resize(edges_.size(), EdgeID::empty);

        // Step 1: compress the vertices.
        remap_vertices(map);

        // Step 2: remap the old VertexID values in edges_
        remap_edge_vertices(map);

        // Step 3: compress the edges.
        remap_edges(map);

        // Step 4: rebuild the topology.
        rebuild_topology();

        return map;
    }

    // This function compresses the graph's vertices to improve
    // performance after many remove operations.
    CompressionMap compress_vertices()
        requires (vertices_mutable && edges_mutable)
    {
        CompressionMap map;
        auto & vertex_map = map.old_to_new_vertex;
        auto & edge_map = map.old_to_new_edge;

        // Set the size (current in use range)
        vertex_map.resize(vertices_.size(), VertexID::empty);
        edge_map.resize(edges_.size(), EdgeID::empty);

        // Step 1: compress the vertices.
        remap_vertices(map);

        // Step 2: remap the old VertexID values in edges_
        remap_edge_vertices(map);

        // Skip step 3, edges are not being compressed.

        // Step 4: rebuild the topology.
        rebuild_topology_safe();

        return map;
    }

    // This function compresses the graph's edges to improve
    // performance after many remove operations.
    CompressionMap compress_edges()
        requires (vertices_mutable && edges_mutable)
    {
        CompressionMap map;
        auto & vertex_map = map.old_to_new_vertex;
        auto & edge_map = map.old_to_new_edge;

        // Set the size (current in use range)
        vertex_map.resize(vertices_.size(), VertexID::empty);
        edge_map.resize(edges_.size(), EdgeID::empty);

        // Skip step 1, vertices are not being compressed.

        // Skip step 2, there was no remapping of vertices.

        // Step 3: compress the edges.
        remap_edges(map);

        // Step 4: rebuild the topology.
        rebuild_topology_safe();

        return map;
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

    // TODO: requires clause, impl
    // Return a lightweight transposed view of the graph.
    [[nodiscard]] decltype(auto) transpose_view() const &
    {
        return TransposeView{*this};
    }

    // Prevent creating a non-owning view of a temporary.
    [[nodiscard]] decltype(auto) transpose_view() const && = delete;

// Internal functions for compression calls.
private:
    // Internal function for compressing and remapping the vertex array.
    void remap_vertices(CompressionMap & map)
        // We std::swap VertexData elements and may destroy dead vertices.
        noexcept(std::is_nothrow_swappable_v<VertexData> &&
                 std::is_nothrow_destructible_v<VertexData>)
    {
        // We want to swap and pop dead vertices with alive vertices
        // until we have removed all of the empty space.

        // Use two pointers, the head looks for dead slots and the tail
        // looks for alive slots to swap.
        size_t head = 0;
        size_t tail = vertices_.size() - 1;

        while (head <= tail)
        {
            // Search for the first dead slot.
            while (head <= tail && vertices_.alive(head))
            {
                // We need to map this vertex to itself, since by default
                // we set the entire vector to VertexID::empty
                map.old_to_new_vertex[head] = head;
                head++;
            }

            // Search backwards for the first alive slot.
            while (head <= tail && !vertices_.alive(tail))
            {
                // Prevent underflow.
                if (tail == 0)
                {
                    break;
                }

                tail--;
            }

            // If we haven't passed over the boundry, swap.
            if (head < tail)
            {
                if constexpr (has_vertex_data)
                {
                    std::swap(vertices_[head],
                              vertices_[tail]);
                }

                vertices_.set_alive(head);
                vertices_.set_dead(tail);

                // Record the mapping.
                map.old_to_new_vertex[tail] = head;

                // Keep the iteration going.
                head++;

                // Prevent underflow.
                if (tail == 0)
                {
                    break;
                }

                tail--;
            }
        }

        // vertex_count = vertices_.size() - free_vertices_.size()
        size_t v_count = vertex_count();

        vertices_.resize(v_count);
        free_vertices_.clear();
    }

    // Internal function for remapping each VertexID stored in an edge.
    void remap_edge_vertices(CompressionMap & map) noexcept
    {
        auto & vertex_map = map.old_to_new_vertex;
        for (size_t i = 0; i < edges_.size(); i++)
        {
            if (valid_edge(i))
            {
                // Apply the stored transformation.
                edges_.from[i] = vertex_map[edges_.from[i].value];
                edges_.to[i] = vertex_map[edges_.to[i].value];
            }
        }
    }

    // Internal function for compressing and remapping the edges array.
    void remap_edges(CompressionMap & map)
        // We std::swap EdgeData elements and may destroy dead vertices.
        noexcept(std::is_nothrow_swappable_v<EdgeData> &&
                 std::is_nothrow_destructible_v<EdgeData>)
    {
        // We want to swap and pop dead edges with alive edges
        // until we have removed all of the empty space.

        // Use two pointers, the head looks for dead slots and the tail
        // looks for alive slots to swap.
        size_t head = 0;
        size_t tail = edges_.size() - 1;

        while (head <= tail)
        {
            // Search for the first dead slot.
            while (head <= tail && edges_.alive(head))
            {
                // We need to map this edge to itself, since by default
                // we set the entire vector to EdgeID::empty
                map.old_to_new_edge[head] = head;
                head++;
            }

            // Search backwards for the first alive slot.
            while (head <= tail && !edges_.alive(tail))
            {
                // Prevent underflow.
                if (tail == 0)
                {
                    break;
                }

                tail--;
            }

            // If we haven't passed over the boundry, swap.
            if (head < tail)
            {
                std::swap(edges_.from[head], edges_.from[tail]);
                std::swap(edges_.to[head], edges_.to[tail]);

                if constexpr (has_edge_data)
                {
                    std::swap(edges_.data[head], edges_.data[tail]);
                }

                // Record the mapping.
                map.old_to_new_edge[tail] = head;

                // Keep the iteration going.
                head++;

                // Prevent underflow.
                if (tail == 0)
                {
                    break;
                }

                tail--;
            }
        }

        // edge_count = edges_.size() - free_edges_.size()
        size_t e_count = edge_count();
        edges_.resize(e_count);
        free_edges_.clear();
    }

    void rebuild_topology()
    {
        auto indices = std::views::iota(size_t{0}, edges_.size());

        auto edge_pairs = indices
                          | std::views::transform([this](size_t i){
                                return EdgeView{edges_.from[i],
                                                edges_.to[i],
                                                i};
                          });

        if constexpr (Rebuildable<Representation, decltype(edge_pairs)>)
        {
            representation_.rebuild(edge_pairs);
        }
        else
        {
            representation_.clear();

            for (auto&& edge : edge_pairs)
            {
                representation_.add_edge(edge.from, edge.to, edge.id);
            }
        }
    }

    void rebuild_topology_safe()
    {
        auto indices = std::views::iota(size_t{0}, edges_.size())
                       | std::views::filter([this](size_t i){
                             return edges_.alive(i);
                       });

        auto edge_pairs = indices
                          | std::views::transform([this](size_t i){
                                return EdgeView{edges_.from[i],
                                                edges_.to[i],
                                                i};
                          });

        if constexpr (Rebuildable<Representation, decltype(edge_pairs)>)
        {
            representation_.rebuild(edge_pairs);
        }
        else
        {
            representation_.clear();

            for (auto&& edge : edge_pairs)
            {
                representation_.add_edge(edge.from, edge.to, edge.id);
            }
        }
    }

private:

    // Internal function for when a representation can be constructed
    // from an edge view and thus we do not want normal add_edge logic.
    //
    // We avoid modifications to the representation entirely.
    template<typename... Args>
    void push_back_edge(VertexID from, VertexID to, Args&&... args)
    {
        auto res = add_edge_impl<EdgeInsertPolicy::PushBack>(
                        from, to, std::forward<Args>(args)...);

        libproject_assert(res.second == true,
                          "PropertyGraph::push_back_edge(from, to): "
                          "internal library error: operation failed; "
                          "from = "
                          + std::to_string(from.value)
                          + " , to = "
                          + std::to_string(to.value));
    }

    // Internal function for removing vertex data from
    // our PropertyGraph edges_ storage.
    //
    // Data must be modified elsewhere.
    void remove_edge(EdgeID e_id)
    {
        edges_.from[e_id.value] = VertexID::empty;
        edges_.to[e_id.value] = VertexID::empty;
        free_edges_.push_back(e_id);
    }

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

        void resize(size_t N)
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

        size_t size() const noexcept
        {
            if constexpr (has_edge_data)
            {
                libproject_assert(from.size() == to.size(),
                                  "PropertyGraph::EdgeArrays::size(): "
                                  "internal library error: from and to arrays "
                                  "differ in size; from.size() = "
                                  + std::to_string(from.size())
                                  + " to.size() = "
                                  + std::to_string(to.size()));

                libproject_assert(data.size() == from.size(),
                                  "PropertyGraph::EdgeArrays::size(): "
                                  "internal library error: dat and from arrays "
                                  "differ in size; data.size() = "
                                  + std::to_string(data.size())
                                  + " from.size() = "
                                  + std::to_string(from.size()));

                return data.size();
            }
            else
            {
                libproject_assert(from.size() == to.size(),
                                  "PropertyGraph::EdgeArrays::size(): "
                                  "internal library error: from and to arrays "
                                  "differ in size; from.size() = "
                                  + std::to_string(from.size())
                                  + " to.size() = "
                                  + std::to_string(to.size()));

                return from.size();
            }
        }

        // If an edge was deleted, edges_.from will be the empty vertex.
        bool alive(EdgeID e_id) const noexcept
        {
            libproject_assert(e_id.value < from.size(),
                              "PropertyGraph::EdgeArrays::alive(e_id): "
                              "internal library error: out of bounds access "
                              "in from array; from.size() = "
                              + std::to_string(from.size())
                              + " e_id = "
                              + std::to_string(e_id.value));

            return from[e_id.value] != VertexID::empty;
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

        void resize(size_t N)
        {
            if constexpr (has_vertex_data)
            {
                data_.resize(N);
            }
            else
            {
                count_ = N;
            }

            if constexpr (vertices_mutable)
            {
                alive_vertices_.resize(N);
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
            noexcept(std::is_nothrow_destructible_v<VertexData>)
        {
            if constexpr (has_vertex_data)
            {
                libproject_assert(data_.size() > 0,
                                  "PropertyGraph::VertexStorage::pop_back(): "
                                  "internal library error: "
                                  "data_ vector is empty");

                data_.pop_back();
            }
            else
            {
                libproject_assert(count_ - 1 > count_,
                                  "PropertyGraph::VertexStorage::pop_back(): "
                                  "internal library error: count_ underflowed");
                count_--;
            }

            if constexpr (vertices_mutable)
            {
                libproject_assert(alive_vertices_.size() > 0,
                                  "PropertyGraph::VertexStorage::pop_back(): "
                                  "internal library error: "
                                  "alive_vertices_ vector is empty");

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

        void increment_counter() noexcept
            requires (vertices_mutable && !has_vertex_data)
        {
            count_++;
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

        VertexData & operator[](size_t i) noexcept
            requires (has_vertex_data)
        {
            libproject_assert(i < data_.size(),
                              "PropertyGraph::VertexStorage::operator[] "
                              "internal library error: out of bounds access; "
                              "data_.size() = " + std::to_string(data_.size())
                              + "index = " + std::to_string(i));

            return data_[i];
        }

        const VertexData & operator[](size_t i) const noexcept
            requires (has_vertex_data)
        {
            libproject_assert(i < data_.size(),
                              "PropertyGraph::VertexStorage::operator[] "
                              "internal library error: out of bounds access; "
                              "data_.size() = " + std::to_string(data_.size())
                              + ", index = " + std::to_string(i));

            return data_[i];
        }

        bool alive(VertexID v) const noexcept
            requires (vertices_mutable)
        {
            libproject_assert(v.value < alive_vertices_.size(),
                              "PropertyGraph::VertexStorage::alive(v) "
                              "internal library error: out of bounds access; "
                              "alive_vertices_.size() = "
                              + std::to_string(alive_vertices_.size())
                              + ", vertex = " + std::to_string(v.value));

            return alive_vertices_[v.value];
        }

        void set_alive(VertexID v) noexcept
            requires (vertices_mutable)
        {
            libproject_assert(v.value < alive_vertices_.size(),
                              "PropertyGraph::VertexStorage::set_alive(v) "
                              "internal library error: out of bounds access; "
                              "alive_vertices_.size() = "
                              + std::to_string(alive_vertices_.size())
                              + ", vertex = " + std::to_string(v.value));

            alive_vertices_[v.value] = 1;
        }

        void set_dead(VertexID v) noexcept
            requires (vertices_mutable)
        {
            libproject_assert(v.value < alive_vertices_.size(),
                              "PropertyGraph::VertexStorage::set_dead(v) "
                              "internal library error: out of bounds access; "
                              "alive_vertices_.size() = "
                              + std::to_string(alive_vertices_.size())
                              + ", vertex = " + std::to_string(v.value));

            alive_vertices_[v.value] = 0;
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
