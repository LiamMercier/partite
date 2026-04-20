#pragma once

#include <ranges>

#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{
    //
    // Concepts for neighborhood queries.
    //

    template <typename Graph>
    concept AdjacencyGraph = requires (const Graph & g, VertexID v)
    {
        { g.neighbors(v) } -> std::ranges::range;

        requires std::same_as<std::ranges::range_value_t
                                <decltype(g.neighbors(v))>,
                                VertexID>;
    };

    template <typename Graph>
    concept BidirectionalAdjacencyGraph = AdjacencyGraph<Graph> &&
                                          requires (const Graph & g, VertexID v)
    {
        { g.in_neighbors(v) } -> std::ranges::range;

        requires std::same_as<std::ranges::range_value_t
                                <decltype(g.in_neighbors(v))>,
                                VertexID>;
    };

    template <typename Graph>
    concept IncidenceGraph = requires (const Graph & g, VertexID v)
    {
        { g.incident_edges(v) } -> std::ranges::range;

        requires std::same_as<std::ranges::range_value_t
                                <decltype(g.in_neighbors(v))>,
                                IncidentEdge>;
    };

    template <typename Graph>
    concept BidirectionalIncidenceGraph = IncidenceGraph<Graph> &&
                                          requires (const Graph & g, VertexID v)
    {
        { g.incident_edges(v) } -> std::ranges::range;

        requires std::same_as<std::ranges::range_value_t
                                <decltype(g.in_neighbors(v))>,
                                IncidentEdge>;
    };
};
