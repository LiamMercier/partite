#pragma once

#include <libproject/internal/libproject-assert.hpp>
#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>

namespace libproject::graphs
{
    template <typename PQ, typename Numeric>
    concept DijkstraCompatiblePQ = requires (PQ & pq,
                                             VertexID v,
                                             Numeric dist,
                                             std::vector<VertexID> vs,
                                             std::vector<Numeric> d,
                                             size_t source)
    {
        { pq.build(vs, d, source) };
        { pq.extract_min() } -> std::convertible_to<VertexID>;
        { pq.decrease_key(v, dist) };
    };

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
                   size_t source)
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
            swap_nodes(source, 0);
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
