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
    { T::is_directed } -> std::convertible_to<bool>;

    // TODO: require lookup, etc.
};

};
