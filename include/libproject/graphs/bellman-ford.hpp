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
        { W::INF } -> std::convertible_to<typename W::numeric_type>;
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
              EdgeListGraph Graph,
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

        if constexpr (Config.release_checks)
        {
            if (source >= graph.vertex_count())
            {
                throw std::out_of_range("source vertex out of range");
            }
        }

        libproject_assert(source < graph.vertex_count(),
                          "bellman_ford(graph, source, w, workspace, visitor): "
                          "source vertex is invalid; source = "
                          + std::to_string(source.value)
                          + ", graph.vertex_count() = "
                          + std::to_string(graph.vertex_count()));

        auto & Mode = Config.mode;

        auto & distance = workspace.distance;
        auto & parent = workspace.parent;

        // If we are doing SSSP instead of SPSP we should
        // initialize the entire array now instead of doing
        // lazy initialization.
        if constexpr (Mode == BellmanFordInitMode::Eager)
        {
            // Initialize for single source shortest paths
            workspace.initialize_single_source(graph.vertex_count());
        }
        else
        {
            workspace.next();
        }

        auto dist = [&](VertexID v) -> Numeric &
        {
            if constexpr (Mode == BellmanFordInitMode::Lazy)
            {
                return workspace.get_dist_lazy(v);
            }
            else
            {
                return workspace.get_dist(v);
            }
        };

        distance[source.value] = 0;

        // If we are using lazy initialization, mark the epoch now.
        if constexpr (Mode == BellmanFordInitMode::Lazy)
        {
            workspace.mark_epoch(source);
        }

        visitor.source_vertex(source);

        // Do |V| - 1 iterations of relaxing each edge
        for (size_t iter = 0; iter < graph.vertex_count(); iter++)
        {
            // Track if we have relaxed any edge this iteration.
            bool relaxed_edge = false;;

            for (const auto & [from, to, e] : graph.edges())
            {
                // Apply any filtering rules.
                if constexpr (ExamineControllable<Visitor>)
                {
                    ControlAction action = visitor.examine_edge(from, to, e);

                    if (action == ControlAction::Skip)
                    {
                        continue;
                    }
                    else if (action == ControlAction::Stop)
                    {
                        return;
                    }
                }
                else
                {
                    visitor.examine_edge(from, to, e);
                }

                // Try to relax this edge. Either of these values
                // might be INF so we need to ensure we do not overflow.
                Numeric d_from = dist(from);
                Numeric d_to = dist(to);

                if (d_from == Workspace::INF)
                {
                    visitor.edge_not_relaxed(from, to, e);
                    continue;
                }

                Numeric candidate_w = d_from + w(e);

                if (d_to > candidate_w)
                {
                    workspace.get_dist(to) = candidate_w;
                    parent[to.value] = from;

                    visitor.edge_relaxed(from, to, e);
                    relaxed_edge = true;
                }
                else
                {
                    visitor.edge_not_relaxed(from, to, e);
                }

            }

            if (!relaxed_edge)
            {
                break;
            }

        }
    }

    // Overload that creates a temporary workspace
    // for users who just want a visitor to run.
    //
    // Users who have large graphs or large weights should
    // instead define the workspace themselves and ensure
    // that the workspace Numeric is large enough to handle
    // whatever type the weight function may return.
    //
    // For example, the weight function could take two
    // size_t values and return a Numeric type that is
    // 128 bits for storage.
    template <EdgeListGraph Graph,
              typename WeightFunction,
              BellmanFordVisitor Visitor>
    void bellman_ford(Graph & graph,
                      VertexID source,
                      WeightFunction w,
                      Visitor&& visitor)
        requires (BellmanFordWeightFunction
                  <WeightFunction,
                   std::remove_cvref_t<
                        std::invoke_result_t<WeightFunction&, EdgeID>>
                  >)
    {
        using WeightType = std::remove_cvref_t<
                               std::invoke_result_t<WeightFunction, EdgeID>
                           >;

        libproject_assert(source < graph.vertex_count(),
                          "bellman_ford(graph, source, w, visitor): "
                          "source vertex is invalid; source = "
                          + std::to_string(source.value)
                          + ", graph.vertex_count() = "
                          + std::to_string(graph.vertex_count()));

        BellmanFordWorkspace<WeightType> workspace(graph.vertex_count());

        bellman_ford(graph, source, w, workspace, visitor);
    }

};
