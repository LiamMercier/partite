#pragma once

#include <ranges>

#include <partite/graphs/graph-constants.hpp>
#include <partite/graphs/graph-inputs.hpp>

// Definition of a graph topology
namespace partite::graphs
{

template <typename T>
concept GraphRepresentation = requires(const T rep)
{
    // Ensure we have properly set Directed.
    { T::directed } -> std::convertible_to<bool>;

    // Ensure we know if the representation support multigraphs.
    { T::multigraph } -> std::convertible_to<bool>;

    // Ensure we can query the max valid vertex.
    { rep.max_vertex_id() } -> std::integral;
};

// To ensure we can construct properly.
template <typename T>
concept PropertyGraphConstructible = GraphRepresentation<T>
                                     && std::constructible_from<T, size_t>;

template <typename T, typename ER>
concept EdgeViewRangeConstructible = PropertyGraphConstructible<T> &&
                                     EdgeViewRange<ER> &&
                                     requires(std::size_t v_count, ER&& edges)
{
    T(v_count, std::forward<ER>(edges));
};

// Representations that allow edge lookups.
template <typename T>
concept EdgeQueryable = GraphRepresentation<T> &&
                        requires(const T rep, VertexID v_id)
{
    { rep.has_edge(v_id, v_id) } -> std::convertible_to<bool>;
} &&
(T::multigraph || requires(const T rep, VertexID v_id)
{
    { rep.find_edge(v_id, v_id) } -> std::convertible_to<EdgeID>;
}) &&
(!T::multigraph || requires(const T rep, VertexID v)
{
    { rep.find_edges(v, v) } -> std::ranges::range;

    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.find_edges(v, v))>,
                          EdgeID>;
});

template <typename T>
concept EdgeCountable = GraphRepresentation<T> &&
                        requires(const T rep, VertexID v)
{
    { rep.edge_count(v, v) } -> std::convertible_to<size_t>;
};

// Representations that allow edge modifications.
template <typename T>
concept EdgeMutable = GraphRepresentation<T> &&
                      requires(T rep, VertexID v, EdgeID e)
{
    { rep.add_edge(v, v, e) } -> std::same_as<EdgeID>;
} &&
(T::multigraph || requires(T rep, VertexID v)
{
    { rep.remove_edge(v, v) } -> std::same_as<EdgeID>;
}) &&
(!T::multigraph || requires(T rep, VertexID v, EdgeID e)
{
    { rep.remove_edge(v, v, e) } -> std::same_as<EdgeID>;
});

// Representations that can be cleared.
template <typename T>
concept Clearable = GraphRepresentation<T> &&
                    requires(T rep, VertexID v_id)
{
    { rep.clear() };
};

//
// Concepts for getting the VertexID of the neighbors of a vertex.
//

template <typename T>
concept NeighborsQueryable = GraphRepresentation<T> &&
                            requires(const T rep, VertexID v_id)
{
    { rep.neighbors(v_id) } -> std::ranges::range;

    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.neighbors(v_id))>,
                          VertexID>;
};

template <typename T>
concept OutNeighborsQueryable = GraphRepresentation<T> &&
                                requires(const T rep, VertexID v_id)
{
    { rep.out_neighbors(v_id) } -> std::ranges::range;

    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.out_neighbors(v_id))>,
                          VertexID>;
};

// For representations that allows in_neighbors(v)
template <typename T>
concept InNeighborsQueryable = GraphRepresentation<T> &&
                               requires(const T rep, VertexID v_id)
{
    { rep.in_neighbors(v_id) } -> std::ranges::range;

    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.in_neighbors(v_id))>,
                          VertexID>;
};

//
// Concepts for getting the IncidentEdge of the neighbors of a vertex.
//

template <typename T>
concept IncidentEdgesQueryable = GraphRepresentation<T> &&
                                 requires(const T rep, VertexID v_id)
{
    { rep.incident_edges(v_id) } -> std::ranges::range;
    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.incident_edges(v_id))>,
                          IncidentEdge>;
};

template <typename T>
concept OutEdgesQueryable = GraphRepresentation<T> &&
                            requires(const T rep, VertexID v_id)
{
    { rep.out_edges(v_id) } -> std::ranges::range;
    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.out_edges(v_id))>,
                          IncidentEdge>;
};

template <typename T>
concept InEdgesQueryable = GraphRepresentation<T> &&
                           requires(const T rep, VertexID v_id)
{
    { rep.in_edges(v_id) } -> std::ranges::range;
    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.in_edges(v_id))>,
                          IncidentEdge>;
};

//
// Concepts for getting the EdgeID of the neighbors of a vertex.
//

template <typename T>
concept IncidentEdgeIDsQueryable = GraphRepresentation<T> &&
                                 requires(const T rep, VertexID v_id)
{
    { rep.incident_edge_ids(v_id) } -> std::ranges::range;
    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.incident_edge_ids(v_id))>,
                          EdgeID>;
};

template <typename T>
concept OutEdgeIDsQueryable = GraphRepresentation<T> &&
                            requires(const T rep, VertexID v_id)
{
    { rep.out_edge_ids(v_id) } -> std::ranges::range;
    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.out_edge_ids(v_id))>,
                          EdgeID>;
};

template <typename T>
concept InEdgeIDsQueryable = GraphRepresentation<T> &&
                           requires(const T rep, VertexID v_id)
{
    { rep.in_edge_ids(v_id) } -> std::ranges::range;
    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.in_edge_ids(v_id))>,
                          EdgeID>;
};

//
// Concepts for degree queries.
//

template <typename T>
concept DegreeQueryable = GraphRepresentation<T> &&
                          requires(const T rep, VertexID v_id)
{
    { rep.degree(v_id) } -> std::convertible_to<std::size_t>;
};

// For representations that allow out_degree(v)
template <typename T>
concept OutDegreeQueryable = GraphRepresentation<T> &&
                             requires(const T rep, VertexID v_id)
{
    { rep.out_degree(v_id) } -> std::convertible_to<std::size_t>;
};

// For representations that allow in_degree(v)
template <typename T>
concept InDegreeQueryable = GraphRepresentation<T> &&
                            requires(const T rep, VertexID v_id)
{
    { rep.in_degree(v_id) } -> std::convertible_to<std::size_t>;
};

// Representations that can be transposed.
template <typename T>
concept Transposable = GraphRepresentation<T> &&
                       std::movable<T> &&
                       requires(const T rep, VertexID v_id)
{
    { rep.compute_transpose() } -> std::same_as<T>;
};

// Representations that expose .rebuild()
template <typename T, typename ER>
concept Rebuildable = GraphRepresentation<T> &&
                      EdgeViewRange<ER> &&
                      requires(T & rep,
                               const ER & edges)
{
    { rep.rebuild(edges) };
};

// Representations where self loops can be checked.
template <typename T>
concept SelfLoopQueryable = GraphRepresentation<T> &&
                            requires(const T rep, VertexID v_id)
{
    { rep.has_self_loops() } -> std::convertible_to<bool>;
    { rep.has_self_loop(v_id) } -> std::convertible_to<bool>;
};

// Representations where a vertex can be checked to be valid.
template <typename T>
concept VertexValidatable = GraphRepresentation<T> &&
                            requires(const T rep, VertexID v_id)
{
    { rep.valid_vertex() } -> std::convertible_to<bool>;
};

// Representations that allow vertex modifications.
template <typename T>
concept VertexMutable = GraphRepresentation<T> &&
                        requires(T & rep, VertexID v_id)
{
    { rep.add_vertex(v_id) } -> std::convertible_to<bool>;

    // Callbacks must be called exactly once per EdgeID we remove.
    { rep.remove_vertex(v_id, [](EdgeID){}) };
};

};
