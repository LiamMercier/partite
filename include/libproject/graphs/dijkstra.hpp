#pragma once

#include <libproject/internal/libproject-assert.hpp>
#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>
#include <libproject/graphs/control-actions.hpp>

namespace libproject::graphs
{
    template <typename PQ, typename Numeric>
    concept DijkstraCompatiblePQ = requires (PQ & pq,
                                             VertexID v,
                                             Numeric dist,
                                             std::vector<VertexID> vs,
                                             std::vector<Numeric> d,
                                             VertexID source,
                                             const uint16_t & epoch,
                                             std::vector<uint16_t> v_epochs)
    {
        { PQ{size_t{0}, epoch, v_epochs} };
        { pq.should_push(v) } -> std::convertible_to<bool>;
        { pq.push(v, dist) };
        { pq.extract_min() } -> std::convertible_to<VertexID>;
        { pq.decrease_key(v, dist) };
        { pq.size() } -> std::convertible_to<size_t>;
        { pq.clear() };
    };

    // A binary heap based priority queue with optimizations
    // under the assumption of being used for dijkstra's algorithm.
    template <typename Numeric>
    struct DijkstraDefaultMinPQ
    {
        static constexpr size_t OUTSIDE_HEAP
            = std::numeric_limits<size_t>::max();

        DijkstraDefaultMinPQ(size_t v_count,
                             const uint16_t & epoch,
                             std::vector<uint16_t> & v_epochs)
        :vertex_to_index(v_count, OUTSIDE_HEAP),
        vertex_epoch(v_epochs),
        current_epoch(epoch)
        {
            heap_vertices.reserve(v_count);
            heap_distances.reserve(v_count);
        }

        size_t size() const
        {
            return heap_vertices.size();
        }

        void resize(size_t N)
        {
            vertex_to_index.resize(N);
        }

        void clear()
        {
            heap_distances.clear();
            heap_vertices.clear();
        }

        size_t parent(size_t i) const
        {
            return (i - 1) / 2;
        }

        size_t left(size_t i) const
        {
            return (2 * i) + 1;
        }

        size_t right(size_t i) const
        {
            return 2 * (i + 1);
        }

        void swap_nodes(size_t i, size_t j)
        {
            std::swap(heap_vertices[i],
                      heap_vertices[j]);

            std::swap(heap_distances[i],
                      heap_distances[j]);

            vertex_to_index[heap_vertices[i].value] = i;
            vertex_to_index[heap_vertices[j].value] = j;
        }

        void heapify(size_t i)
        {
            size_t n = size();
            size_t curr = i;

            while (true)
            {
                size_t l = left(curr);
                size_t r = right(curr);

                size_t smallest = curr;

                if (l < n && heap_distances[l] < heap_distances[smallest])
                {
                    smallest = l;
                }
                if (r < n && heap_distances[r] < heap_distances[smallest])
                {
                    smallest = r;
                }

                if (smallest == curr)
                {
                    break;
                }

                swap_nodes(curr, smallest);
                curr = smallest;
            }
        }

        // TODO <optimization>: move into hole vs swap.
        void sift_up(size_t i)
        {
            while ((i > 0)
                   && (heap_distances[i] < heap_distances[parent(i)]))
            {
                swap_nodes(i, parent(i));
                i = parent(i);
            }
        }

        //
        // Begin required functions.
        //

        bool should_push(VertexID v) const
        {
            if (vertex_epoch[v.value] != current_epoch)
            {
                return true;
            }

            return false;
        }

        void push(VertexID v, Numeric dist)
        {
            size_t i = size();
            heap_vertices.push_back(v);
            heap_distances.push_back(dist);

            vertex_to_index[v.value] = i;

            sift_up(i);
        }

        VertexID extract_min()
        {
            size_t n = size();

            libproject_assert(n > 0,
                              "DijkstraDefaultMinPQ::extract_min(): "
                              "internal library error: tried to extract from "
                              "an empty heap");

            VertexID min = heap_vertices[0];

            if (n == 1)
            {
                heap_vertices.pop_back();
                heap_distances.pop_back();
#ifndef NDEBUG
                vertex_to_index[min.value] = OUTSIDE_HEAP;
#endif

                return min;
            }

            heap_vertices[0] = heap_vertices[n - 1];
            heap_distances[0] = heap_distances[n - 1];

            heap_vertices.pop_back();
            heap_distances.pop_back();

#ifndef NDEBUG
            vertex_to_index[min.value] = OUTSIDE_HEAP;
#endif
            // update the mapping for the vertex
            // which hasn't been used yet.
            vertex_to_index[heap_vertices[0].value] = 0;

            heapify(0);

            return min;
        }

        void decrease_key(VertexID v, Numeric dist)
        {
            // find the index where v exists.
            size_t i = vertex_to_index[v.value];

            // Theoretically impossible, dijkstra will never try
            // to decrease key on a node that was extracted since
            // its SSSP was already found.
            libproject_assert(i != OUTSIDE_HEAP,
                              "DijkstraDefaultMinPQ::decrease_key(v, dist): "
                              "tried to decrease key for vertex "
                              "outside the heap; v = "
                              + std::to_string(v.value)
                              + "\nThis indicates:\n"
                              "(1) a negative weight (invariant violation)\n"
                              "(2) workspace reused without epoch increment "
                              "(stale state).");

            libproject_assert(i < size(),
                              "DijkstraDefaultMinPQ::decrease_key(v, dist): "
                              "internal library error: tried to access heap "
                              "index out of bounds; v = "
                              + std::to_string(v.value)
                              + ", index = "
                              + std::to_string(i)
                              + ", size = "
                              + std::to_string(size()));

            libproject_assert(heap_distances[i] > dist,
                              "DijkstraDefaultMinPQ::decrease_key(v, dist): "
                              "internal library error: tried to decrease "
                              "key to a larger key; previous key = "
                              + std::to_string(heap_distances[i])
                              + ", dist = "
                              + std::to_string(dist));

            // update its key.
            heap_distances[i] = dist;

            // maintain the invariant.
            sift_up(i);
        }

        std::vector<VertexID> heap_vertices;
        std::vector<Numeric> heap_distances;

        // We already need Theta(|V|) space for the two other
        // arrays, so this is reasonable for performance
        // versus using a map.
        std::vector<size_t> vertex_to_index;
        std::vector<uint16_t> & vertex_epoch;
        const uint16_t & current_epoch{0};
    };

    // Optional struct for a default usable workspace.
    //
    // Numeric's max must be able to store the maximum
    // possible path length in the graph.
    template <typename Numeric,
              DijkstraCompatiblePQ<Numeric> PQ
                  = DijkstraDefaultMinPQ<Numeric>>
    struct DijkstraWorkspace
    {
        using numeric_type = Numeric;

        static constexpr numeric_type INF
            = std::numeric_limits<numeric_type>::max();

        DijkstraWorkspace(size_t vertex_count)
        :current_capacity(vertex_count),
        vertex_epoch(vertex_count, 0),
        pq(vertex_count, current_epoch, vertex_epoch)
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
            pq.resize(N);

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

            pq.clear();
        }

        // Theta(V) preallocation logic if pure SSSP
        void initialize_single_source(size_t vertex_count)
        {
            next();

            // nice utility instead of fill(ptr, ptr +  count, val)
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
        PQ pq;

        // To avoid having to run an O(V) initialization
        // on this memory during construction.
        std::unique_ptr<Numeric[]> distance;
        std::unique_ptr<VertexID[]> parent;
    };

    template <typename W>
    concept DijkstraCompatibleWorkspace = requires (W & workspace,
                                                    VertexID v)
    {
        // Ensure the type can be used for addition.
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

        // Necessary members.
        { workspace.distance };
        { workspace.parent };

        // Check that workspace.pq is compatible.
        requires DijkstraCompatiblePQ<
            std::remove_reference_t<decltype(workspace.pq)>,
            typename W::numeric_type
        >;
    };

    template <typename V>
    concept DijkstraVisitor = requires(V & visitor,
                                       VertexID u,
                                       VertexID v,
                                       EdgeID e)
    {
        { visitor.source_vertex(u) };
        { visitor.expand_vertex(u) };
        { visitor.examine_edge(u, v, e) };
        { visitor.edge_relaxed(u, v, e) };
        { visitor.edge_not_relaxed(u, v, e) };
        { visitor.finish_vertex(u) };
    };

    struct NoOpDijkstraVisitor
    {
        void source_vertex(VertexID u)
        {}

        void expand_vertex(VertexID u)
        {}

        void examine_edge(VertexID u, VertexID v, EdgeID e)
        {}

        void edge_relaxed(VertexID u, VertexID v, EdgeID e)
        {}

        void edge_not_relaxed(VertexID u, VertexID v, EdgeID e)
        {}

        void finish_vertex(VertexID u)
        {}
    };

    template <typename W, typename Numeric>
    concept DijkstraWeightFunction = requires (W & w, EdgeID e, Numeric dist)
    {
        { w(e) } -> std::convertible_to<Numeric>;
        { dist + w(e) } -> std::convertible_to<Numeric>;
    };

    enum class DijkstraInitMode : uint8_t
    {
        Lazy,
        Eager
    };

    //
    // Dijkstra function bodies.
    //

    // Generic dijkstra implementation.
    template <DijkstraInitMode Mode = DijkstraInitMode::Eager,
              IncidenceGraph Graph,
              typename WeightFunction,
              DijkstraCompatibleWorkspace Workspace,
              DijkstraVisitor Visitor = NoOpDijkstraVisitor>
    void dijkstra(Graph & graph,
                  VertexID source,
                  WeightFunction w,
                  Workspace & workspace,
                  Visitor&& visitor = Visitor())
        requires (DijkstraWeightFunction
                  <WeightFunction, typename Workspace::numeric_type>)
    {
        using Numeric = typename Workspace::numeric_type;

        auto & distance = workspace.distance;
        auto & parent = workspace.parent;
        auto & pq = workspace.pq;

        // If we are doing SSSP instead of SPSP we should
        // initialize the entire array now instead of doing
        // lazy initialization.
        if constexpr (Mode == DijkstraInitMode::Eager)
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
            if constexpr (Mode == DijkstraInitMode::Lazy)
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
        if constexpr (Mode == DijkstraInitMode::Lazy)
        {
            workspace.mark_epoch(source);
        }

        pq.push(source, 0);

        visitor.source_vertex(source);

        // While the queue is not empty, extract
        while (pq.size() > 0)
        {
            VertexID min = pq.extract_min();

            // After a node has been extracted, it will never be
            // updated again, its SSSP distance has been determined.
            //
            // Apply any filtering or early stopping rules.
            if constexpr (ExpandControllable<Visitor>)
            {
                ControlAction action = visitor.expand_vertex(min);

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
                visitor.expand_vertex(min);
            }

            for (const auto & [to, e] : graph.incident_edges(min))
            {
                // Apply any filtering rules.
                if constexpr (ExamineControllable<Visitor>)
                {
                    ControlAction action = visitor.examine_edge(min, to, e);

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
                    visitor.examine_edge(min, to, e);
                }

                // If min was popped, its distance is not infinite
                // and its epoch was set to the current epoch, so
                // we should always be able to use get_dist instead
                // of the lazy variant that checks the epoch.
                auto candidate_w = workspace.get_dist(min) + w(e);

                // we may have never seen to before, so we should
                // use the lazy variant if necessary.
                //
                // If we are using lazy init, then distance[to.value]
                // is now infinite after any call to get_dist_lazy.
                Numeric d_to = dist(to);

                // If shorter, relax.
                if (d_to > candidate_w)
                {
                    workspace.get_dist(to) = candidate_w;
                    parent[to.value] = min;

                    visitor.edge_relaxed(min, to, e);

                    // Decide if we should push this vertex
                    // because we saw it for the first time, or
                    // if we should decrease the key.
                    if (pq.should_push(to))
                    {
                        workspace.mark_epoch(to);

                        pq.push(to, workspace.get_dist(to));
                    }
                    else
                    {
                        pq.decrease_key(to, workspace.get_dist(to));
                    }
                }
                else
                {
                    visitor.edge_not_relaxed(min, to, e);
                }
            }

            visitor.finish_vertex(min);
        }
    }

    // Overload that creates a temporary workspace
    // for users who just want a visitor to run.
    //
    // We let the weight function determine the Numeric
    // type, since forcing dijkstra<NumericType>(...) as
    // the syntax defeats the utility of this overload.
    //
    // Users who have large graphs or large weights should
    // instead define the workspace themselves and ensure
    // that the workspace Numeric is large enough to handle
    // whatever type the weight function may add each time.
    //
    // For example, declaring a workspace with some 128-bit
    // arithmetic for storage, but size_t as the weight type.
    template <IncidenceGraph Graph,
              typename WeightFunction,
              DijkstraVisitor Visitor>
    void dijkstra(Graph & graph,
                  VertexID source,
                  WeightFunction w,
                  Visitor&& visitor)
        requires (DijkstraWeightFunction
                  <WeightFunction,
                   std::remove_cvref_t<
                        std::invoke_result_t<WeightFunction&, EdgeID>>
                  >)
    {
        using WeightType = std::remove_cvref_t<
                               std::invoke_result_t<WeightFunction, EdgeID>
                           >;

        DijkstraWorkspace<WeightType> workspace(graph.vertex_count());

        dijkstra(graph, source, w, workspace, visitor);
    }

    // TODO <feature>: bidirectional_dijkstra

    //
    // Visitor setups that can be reused.
    //

    template <typename SearchPredicate>
    struct DijkstraSearchVisitor : NoOpDijkstraVisitor
    {
        explicit DijkstraSearchVisitor(SearchPredicate p)
        :found(std::move(p))
        {}

        // When a vertex is expanded, its distance is set in place.
        ControlAction expand_vertex(VertexID v)
            requires std::predicate<SearchPredicate, VertexID>
        {
            if (found(v))
            {
                target_found = v;
                return ControlAction::Stop;
            }

            return ControlAction::Continue;
        }

        void expand_vertex(VertexID v)
            requires (!std::predicate<SearchPredicate, VertexID>)
        {
        }

        VertexID target_found{VertexID::empty};
        SearchPredicate found;
    };

    //
    // dijkstra utility functions.
    //

    // For users who only care about the distance between
    // a pair of points in the graph (single pair shortest distance).
    template <IncidenceGraph Graph,
              typename WeightFunction,
              DijkstraCompatibleWorkspace Workspace>
    void dijkstra_single_pair(Graph & graph,
                              VertexID source,
                              VertexID target,
                              WeightFunction w,
                              Workspace & workspace)
        requires (DijkstraWeightFunction
                  <WeightFunction, typename Workspace::numeric_type>)
    {
        DijkstraSearchVisitor visitor(
            [target](VertexID v){ return v == target; }
        );

        dijkstra<DijkstraInitMode::Lazy>(graph, source, w, workspace, visitor);
    }

    // Temporary workspace overload.
    template <IncidenceGraph Graph,
              typename WeightFunction>
    auto dijkstra_single_pair(Graph & graph,
                              VertexID source,
                              VertexID target,
                              WeightFunction w)
        requires (DijkstraWeightFunction
                  <WeightFunction,
                   std::remove_cvref_t<
                        std::invoke_result_t<WeightFunction&, EdgeID>>
                  >)
    {
        using WeightType = std::remove_cvref_t<
                               std::invoke_result_t<WeightFunction, EdgeID>
                           >;

        DijkstraWorkspace<WeightType> workspace(graph.vertex_count());

        dijkstra_single_pair(graph, source, target, w, workspace);

        return std::pair{std::move(workspace.distance),
                         std::move(workspace.parent)};
    }

};
