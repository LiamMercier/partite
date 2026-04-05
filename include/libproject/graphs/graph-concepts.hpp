#pragma once

#include <ranges>

#include <libproject/graphs/graph-constants.hpp>

// Definition of a graph topology
namespace libproject::graphs
{

template <typename T>
concept GraphRepresentation = requires(const T rep)
{
    // TODO: consider max_vertex_id and then delegate
    //       vertex count to PropertyGraph

    // Ensure we have properly set Directed.
    { T::directed } -> std::convertible_to<bool>;

    // Ensure we can query the number of vertices
    { rep.vertex_count() } -> std::integral;
};

// Check if EdgeData or VertexData is monostate.
template <typename T>
concept HasData = !std::is_same_v<T, std::monostate>;

// To ensure we can construct properly.
template <typename T>
concept PropertyGraphConstructible = GraphRepresentation<T>
                                     && std::constructible_from<T, size_t>;

// Representations that allow edge lookups.
template <typename T>
concept EdgeQueryable = GraphRepresentation<T> &&
                        requires(const T rep, VertexID v_id)
{
    { rep.has_edge(v_id, v_id) } -> std::convertible_to<bool>;
    { rep.find_edge(v_id, v_id) } -> std::same_as<EdgeID>;
};

// Representations that allow edge modifications.
template <typename T>
concept EdgeMutable = GraphRepresentation<T> &&
                      requires(T rep, VertexID v_id, EdgeID e_id)
{
    { rep.add_edge(v_id, v_id, e_id) } -> std::same_as<EdgeID>;
    { rep.remove_edge(v_id, v_id) } -> std::same_as<EdgeID>;
};

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

// Representations that expose a global edge range.
template <typename T>
concept GlobalEdgesQueryable = GraphRepresentation<T> &&
                               requires(const T rep)
{
    { rep.edges() } -> std::ranges::range;
    requires std::same_as<std::ranges::range_value_t
                            <decltype(rep.edges())>,
                          EdgeID>;
};

// Representations that can be transposed.
template <typename T>
concept Transposable = GraphRepresentation<T> &&
                       std::movable<T> &&
                       requires(const T rep, VertexID v_id)
{
    { rep.compute_transpose() } -> std::same_as<T>;
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
// TODO: perhaps split into remove and add
template <typename T>
concept VertexMutable = GraphRepresentation<T> &&
                        requires(T rep, VertexID v_id)
{
    { rep.add_vertex(v_id) } -> std::convertible_to<bool>;
    { rep.remove_vertex(v_id) };
};

};
