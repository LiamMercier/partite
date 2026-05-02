#pragma once

#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>

namespace libproject::graphs
{
    template <typename PQ, typename Numeric>
    concept DijkstraCompatiblePQ = requires (PQ & pq, VertexID v, Numeric dist)
    {
        { pq.insert(v, dist) };
        { pq.extract_min() } -> std::convertible_to<VertexID>;
        { pq.decrease_key(v, dist) };
    };

    template <typename Numeric>
    struct DijkstraDefaultMinHeap
    {
        // TODO: later
    };

    // Optional struct for a default usable workspace.
    //
    // Numeric's max must be able to store the maximum
    // possible path length in the graph.
    template <typename Numeric,
              DijkstraCompatiblePQ<Numeric> PQ
                  = DijkstraDefaultMinHeap<Numeric>>
    struct DijkstraWorkspace
    {
        using numeric_type = Numeric;

        PQ pq;
        std::vector<Numeric> distance;
        std::vector<VertexID> parent;
    };

    template <typename W>
    concept DijkstraCompatibleWorkspace = requires (W & workspace)
    {
        typename W::numeric_type;
        { workspace.distance };
        { workspace.parent };

        // Check that workspace.pq is compatible.
        requires DijkstraCompatiblePQ<
            std::remove_reference_t<decltype(workspace.pq)>,
            typename W::numeric_type
        >;
    };

    // TODO: fill in hook functions later once we sketch the algorithm
    struct NoOpDijkstraVisitor
    {
    };

    template <DijkstraCompatibleWorkspace Workspace>
    struct StandardDijkstraVisitor : NoOpDijkstraVisitor
    {
        StandardDijkstraVisitor(Workspace & w)
        :workspace(w)
        {}

        // TODO: relaxation hooks after sketch

        Workspace & workspace;
    };

};
