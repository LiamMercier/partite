#pragma once

#include <ranges>

#include <libproject/graphs/graph-constants.hpp>

// Definition of a graph topology
namespace libproject::graphs
{

template <typename T>
concept GraphRepresentation = requires(T rep,
                                       VertexID v_id,
                                       EdgeID e_id)
{
    // Ensure we have properly set Directed.
    { T::is_directed } -> std::convertible_to<bool>;

    // Ensure we can query the number of vertices
    { rep.vertex_count() } -> std::integral;

    // Ensure we can iterate on neighbors.
    { rep.neighbors(v_id) } -> std::ranges::range;
};

// TODO: other requirements, maybe queryable

template <GraphRepresentation T>
concept EdgeModifiableRepresentation = requires(T rep,
                                                VertexID v_id,
                                                EdgeID e_id)
{
    { rep.add_edge(v_id, v_id) };
    { rep.remove_edge(v_id, v_id) };
};

};
