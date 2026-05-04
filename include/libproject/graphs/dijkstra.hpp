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
                                             VertexID source)
    {
        { pq.build(vs, d, source) };
        { pq.extract_min() } -> std::convertible_to<VertexID>;
        { pq.decrease_key(v, dist) };
    };

    // A binary heap based priority queue with optimizations
    // under the assumption of being used for dijkstra's algorithm.
    template <typename Numeric>
    struct DijkstraDefaultMinPQ
    {
        DijkstraDefaultMinPQ() = default;

        size_t size() const
        {
            return heap_vertices.size();
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

            size_t l = left(i);
            size_t r = right(i);

            size_t smallest = i;

            if (l < n && heap_distances[l] < heap_distances[smallest])
            {
                smallest = l;
            }
            if (r < n && heap_distances[r] < heap_distances[smallest])
            {
                smallest = r;
            }

            if (smallest != i)
            {
                swap_nodes(i, smallest);
                heapify(smallest);
            }
        }

        void build(std::vector<VertexID> v,
                   std::vector<Numeric> d,
                   VertexID source)
        {
            // Move in the data arrays
            heap_vertices = std::move(v);
            heap_distances = std::move(d);

            // Setup the identity map.
            vertex_to_index.resize(size());

            for (size_t i = 0; i < size(); i++)
            {
                vertex_to_index[heap_vertices[i].value] = i;
            }

            // swap A[source] and A[0] since we know the source
            // is the root element and everything else is equal.
            swap_nodes(static_cast<size_t>(source.value), 0);
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

                return min;
            }

            heap_vertices[0] = heap_vertices[n - 1];
            heap_distances[0] = heap_distances[n - 1];

            heap_vertices.pop_back();
            heap_distances.pop_back();

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
            while ((i > 0)
                   && (heap_distances[i] < heap_distances[parent(i)]))
            {
                swap_nodes(i, parent(i));
                i = parent(i);
            }
        }

        std::vector<VertexID> heap_vertices;
        std::vector<Numeric> heap_distances;

        // We already need Theta(|V|) space for the two other
        // arrays, so this is reasonable for performance
        // versus using a map.
        std::vector<size_t> vertex_to_index;
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

        DijkstraWorkspace(size_t vertex_count)
        :distance(vertex_count),
        parent(vertex_count)
        {}

        void resize(size_t N)
        {
            distance.resize(N);
            parent.resize(N);
        }

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

    //
    // Dijkstra function bodies.
    //

    // Generic dijkstra implementation.
    template <IncidenceGraph Graph,
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
        auto & distance = workspace.distance;
        auto & parent = workspace.parent;
        auto & pq = workspace.pq;

        // Initialize for single source shortest paths
        for (const VertexID & v : graph.vertices())
        {
            distance[v.value] = std::numeric_limits<
                                    typename Workspace::numeric_type
                                >::max();
            parent[v.value] = VertexID::empty;
        }

        distance[source.value] = 0;

        // Gather data for our priority queue. We want two vectors,
        // where heap_verts[i] and dist_copy[i] corresponds to
        // some {VertexID, dist} in the heap.
        //
        // Initially, we have heap_verts[i] = VertexID{i} and
        // dist_copy[i] = distance[i] = distance for VertexID{i}
        std::vector<VertexID> heap_verts(graph.vertex_count());
        std::iota(heap_verts.begin(), heap_verts.end(), 0);
        auto dist_copy = workspace.distance;

        // Build the priority queue from our {VertexID, Distance} data.
        pq.build(heap_verts, dist_copy, source);

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

                auto candidate_w = distance[min.value] + w(e);

                // If shorter, relax.
                if (distance[to.value] > candidate_w)
                {
                    distance[to.value] = candidate_w;
                    parent[to.value] = min;

                    visitor.edge_relaxed(min, to, e);

                    pq.decrease_key(to, distance[to.value]);
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

    // TODO: bidirectional_dijkstra

    //
    // Visitor setups that can be reused.
    //

    template <typename SearchPredicate>
    struct TargetDijkstraVisitor : NoOpDijkstraVisitor
    {
        explicit TargetDijkstraVisitor(SearchPredicate p)
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

    // TODO: A* search with consistent heuristic (need asserts).
    //
    //       Will need to review definitions for this one.

    auto dijkstra_single_pair()
    {

    }

};
