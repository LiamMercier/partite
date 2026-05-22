#pragma once

#include <ranges>

#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{
    template <typename Graph>
    concept DirectedGraph = Graph::directed;

    template <typename Graph>
    concept UndirectedGraph = !DirectedGraph<Graph>;

    template <typename Graph>
    concept BasicGraph = requires (const Graph & g)
    {
        { Graph::directed } -> std::convertible_to<bool>;

        { g.vertex_count() } -> std::convertible_to<size_t>;
    };

    template <typename Graph>
    concept Multigraph = requires ()
    {
        { Graph::multigraph } -> std::convertible_to<bool>;
    };

    //
    // Concepts for neighborhood queries.
    //

    template <typename Graph>
    concept AdjacencyGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.neighbors(v) } -> std::ranges::range;

            requires std::same_as<std::ranges::range_value_t
                                    <decltype(g.neighbors(v))>,
                                    VertexID>;
        };

    template <typename Graph>
    concept InAdjacencyGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.in_neighbors(v) } -> std::ranges::range;

            requires std::same_as<std::ranges::range_value_t
                                    <decltype(g.in_neighbors(v))>,
                                    VertexID>;
        };

    template <typename Graph>
    concept BidirectionalAdjacencyGraph = AdjacencyGraph<Graph>
                                          && (InAdjacencyGraph<Graph>
                                             || UndirectedGraph<Graph>);

    template <typename Graph>
    concept IncidenceGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.incident_edges(v) } -> std::ranges::range;

            requires std::same_as<std::ranges::range_value_t
                                    <decltype(g.incident_edges(v))>,
                                    IncidentEdge>;
        };

    template <typename Graph>
    concept InIncidenceGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.in_edges(v) } -> std::ranges::range;

            requires std::same_as<std::ranges::range_value_t
                                    <decltype(g.in_edges(v))>,
                                    IncidentEdge>;
        };

    template <typename Graph>
    concept BidirectionalIncidenceGraph = IncidenceGraph<Graph>
                                          && (InIncidenceGraph<Graph>
                                             || UndirectedGraph<Graph>);

    template <typename Graph>
    concept DegreeGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.degree(v) } -> std::convertible_to<size_t>;
        };

    template <typename Graph>
    concept OutDegreeGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.out_degree(v) } -> std::convertible_to<size_t>;
        };

    template <typename Graph>
    concept InDegreeGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.in_degree(v) } -> std::convertible_to<size_t>;
        };

    template <typename Graph>
    concept BidirectionalDegreeGraph = OutDegreeGraph<Graph>
                                       && (InDegreeGraph<Graph>
                                          || UndirectedGraph<Graph>);

    template <typename Graph>
    concept VertexListGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.vertices() } -> std::ranges::range;

            requires std::same_as<std::ranges::range_value_t
                                    <decltype(g.vertices())>,
                                    VertexID>;
        };

    template <typename Graph>
    concept EdgeListGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.edges() } -> std::ranges::range;

            requires std::same_as<std::ranges::range_value_t
                                    <decltype(g.edges())>,
                                    EdgeView>;
        };

    template <typename Graph>
    concept EdgeIDListGraph = BasicGraph<Graph> &&
        requires (const Graph & g, VertexID v)
        {
            { g.edge_ids() } -> std::ranges::range;

            requires std::same_as<std::ranges::range_value_t
                                    <decltype(g.edge_ids())>,
                                    EdgeID>;
        };

    template <typename Graph>
    concept VertexMutableGraph =
        BasicGraph<Graph>
        &&
        requires (Graph & g, VertexID v)
        {
            { g.remove_vertex(v) };
        }
        &&
        (requires (Graph & g, VertexID v)
        {
            { g.add_vertex() } -> std::same_as<VertexID>;
        }
        ||
        requires (Graph & g, VertexID v)
        {
            { g.add_vertex(
                std::declval<typename Graph::VertexData>())
            } -> std::same_as<VertexID>;
        });

    template <typename Graph>
    concept EdgeMutableGraph = BasicGraph<Graph> &&
        requires (Graph & g, VertexID v)
        {
            { g.add_edge(v, v) } -> std::same_as<std::pair<EdgeID, bool>>;
            { g.remove_edge(v, v) } -> std::same_as<bool>;
        };

};
