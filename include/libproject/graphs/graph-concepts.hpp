#pragma once

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

    // Ensure lookup.
    { rep.find_edge(v_id, v_id) };
    { rep.has_edge(v_id, v_id) };

};

};
