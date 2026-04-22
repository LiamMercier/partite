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
    concept InAdjacencyGraph = requires (const Graph & g, VertexID v)
    {
        { g.in_neighbors(v) } -> std::ranges::range;

        requires std::same_as<std::ranges::range_value_t
                                <decltype(g.in_neighbors(v))>,
                                VertexID>;
    };

    template <typename Graph>
    concept BidirectionalAdjacencyGraph = AdjacencyGraph<Graph>
                                          && InAdjacencyGraph<Graph>;

    template <typename Graph>
    concept IncidenceGraph = requires (const Graph & g, VertexID v)
    {
        { g.incident_edges(v) } -> std::ranges::range;

        requires std::same_as<std::ranges::range_value_t
                                <decltype(g.incident_edges(v))>,
                                IncidentEdge>;
    };

    template <typename Graph>
    concept InIncidenceGraph = requires (const Graph & g, VertexID v)
    {
        { g.in_edges(v) } -> std::ranges::range;

        requires std::same_as<std::ranges::range_value_t
                                <decltype(g.in_edges(v))>,
                                IncidentEdge>;
    };

    template <typename Graph>
    concept BidirectionalIncidenceGraph = IncidenceGraph<Graph>
                                          && InIncidenceGraph<Graph>;

    template <typename Graph>
    concept DegreeGraph = requires (const Graph & g, VertexID v)
    {
        { g.degree(v) } -> std::convertible_to<size_t>;
    };

    template <typename Graph>
    concept OutDegreeGraph = requires (const Graph & g, VertexID v)
    {
        { g.out_degree(v) } -> std::convertible_to<size_t>;
    };

    template <typename Graph>
    concept InDegreeGraph = requires (const Graph & g, VertexID v)
    {
        { g.in_degree(v) } -> std::convertible_to<size_t>;
    };

    template <typename Graph>
    concept BidirectionalDegreeGraph = OutDegreeGraph<Graph>
                                       && InDegreeGraph<Graph>;

    // TODO: mutable, static, standard graph, etc when needed
};
