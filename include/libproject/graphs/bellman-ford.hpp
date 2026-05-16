#pragma once

#include <libproject/internal/libproject-assert.hpp>
#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>
#include <libproject/graphs/control-actions.hpp>

namespace libproject::graphs
{
    // Optional struct for a default usable workspace.
    //
    // Numeric's max must be able to store the maximum
    // possible path length in the graph.
    template <typename Numeric>
    struct BellmanFordWorkspace
    {
        using numeric_type = Numeric;

        static constexpr numeric_type INF
            = std::numeric_limits<numeric_type>::max();

        BellmanFordWorkspace(size_t vertex_count)
        :current_capacity(vertex_count),
        vertex_epoch(vertex_count, 0)
        {
            distance = std::make_unique_for_overwrite<Numeric[]>(vertex_count);
            parent = std::make_unique_for_overwrite<VertexID[]>(vertex_count);
            next();
        }

        void resize(size_t N)
        {
            if (N <= current_capacity)
            {
                return;
            }

            distance = std::make_unique_for_overwrite<Numeric[]>(N);
            parent = std::make_unique_for_overwrite<VertexID[]>(N);

            vertex_epoch.assign(N, 0);

            current_capacity = N;
            current_epoch = 0;
        }

        void epoch_rollover()
        {
            std::fill(vertex_epoch.begin(),
                      vertex_epoch.end(),
                      0);
        }

        void next()
        {
            current_epoch++;

            if (current_epoch == 0)
            {
                epoch_rollover();
                current_epoch = 1;
            }
        }

        // Theta(V) preallocation logic if pure SSSP
        void initialize_single_source(size_t vertex_count)
        {
            next();

            std::fill_n(distance.get(), vertex_count, INF);
            std::fill_n(parent.get(), vertex_count, VertexID::empty);
        }

        Numeric & get_dist(VertexID v)
        {
            return distance[v.value];
        }

        Numeric & get_dist_lazy(VertexID v)
        {
            if (vertex_epoch[v.value] != current_epoch)
            {
                distance[v.value] = INF;
                parent[v.value] = VertexID::empty;
            }

            return distance[v.value];
        }

        void mark_epoch(VertexID v)
        {
            vertex_epoch[v.value] = current_epoch;
        }

        size_t current_capacity;
        uint16_t current_epoch{0};
        std::vector<uint16_t> vertex_epoch;

        // To avoid having to run an O(V) initialization
        // on this memory during construction.
        std::unique_ptr<Numeric[]> distance;
        std::unique_ptr<VertexID[]> parent;
    };

    template <typename W>
    concept BellmanFordCompatibleWorkspace = requires (W & workspace,
                                                       VertexID v)
    {
        typename W::numeric_type;

        requires requires (typename W::numeric_type a,
                           typename W::numeric_type b)
        {
            { a + b } -> std::same_as<typename W::numeric_type>;
        };

        // Initialization functions
        { workspace.next() };
        { workspace.initialize_single_source(size_t{0}) };

        // Accessors
        { workspace.get_dist(v) } -> std::convertible_to<
                                        typename W::numeric_type>;
        { workspace.get_dist_lazy(v) } -> std::convertible_to<
                                            typename W::numeric_type>;

        // Epoch marking function.
        { workspace.mark_epoch(v) };

        // Necessary members.
        { workspace.distance };
        { workspace.parent };
    };

    template <typename V>
    concept BellmanFordVisitor = requires(V & visitor,
                                          VertexID u,
                                          VertexID v,
                                          EdgeID e)
    {
        { visitor.source_vertex(u) };
        { visitor.examine_edge(u, v, e) };
        { visitor.edge_relaxed(u, v, e) };
        { visitor.edge_not_relaxed(u, v, e) };
    };

    struct NoOpBellmanFordVisitor
    {
        void source_vertex(VertexID u)
        {}

        void examine_edge(VertexID u, VertexID v, EdgeID e)
        {}

        void edge_relaxed(VertexID u, VertexID v, EdgeID e)
        {}

        void edge_not_relaxed(VertexID u, VertexID v, EdgeID e)
        {}
    };

    template <typename W, typename Numeric>
    concept BellmanFordWeightFunction = requires (W & w, EdgeID e, Numeric dist)
    {
        { w(e) } -> std::convertible_to<Numeric>;
        { dist + w(e) } -> std::convertible_to<Numeric>;
    };

    enum class BellmanFordInitMode : uint8_t
    {
        Lazy,
        Eager
    };

    struct BellmanFordExecutionConfig
    {
        constexpr BellmanFordExecutionConfig(BellmanFordInitMode m,
                                             bool rta)
        :mode(m),
        release_checks(rta)
        {}

        constexpr BellmanFordExecutionConfig(BellmanFordInitMode m)
        :mode(m),
        release_checks(false)
        {}

        constexpr BellmanFordExecutionConfig(bool rta)
        :mode(BellmanFordInitMode::Eager),
        release_checks(rta)
        {}

        BellmanFordInitMode mode;
        bool release_checks;
    };

    //
    // Bellman-Ford function bodies.
    //

    // TODO: graph constraints for graph types

    // Generic bellman ford implementation.
    template <BellmanFordExecutionConfig Config
              = BellmanFordExecutionConfig{BellmanFordInitMode::Eager, false},
              typename Graph,
              typename WeightFunction,
              BellmanFordCompatibleWorkspace Workspace,
              BellmanFordVisitor Visitor = NoOpBellmanFordVisitor>
    void bellman_ford(Graph & graph,
                      VertexID source,
                      WeightFunction w,
                      Workspace & workspace,
                      Visitor&& visitor = Visitor())
        requires (BellmanFordWeightFunction
                  <WeightFunction, typename Workspace::numeric_type>)
    {
        using Numeric = typename Workspace::numeric_type;

    }

};
