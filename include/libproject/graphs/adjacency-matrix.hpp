#pragma once
#include <vector>

#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{

// TODO: should we even allow dynamic vertices in this setup?
//
//       Probably not, we have Theta(V^2) reallocation, or Theta(V)
//       amortized reallocation if we expand in a way that every
//       V additions we need to realloc. Seems like no solid use case.

// TODO: Directed should control if we can apply optimizations, such as
//       using half as much memory for graphs vs digraphs.
template <bool Directed>
class AdjacencyMatrix
{
public:
    static constexpr bool is_directed = Directed;

    AdjacencyMatrix(size_t num_vertices)
    :vertex_count_(num_vertices)
    {
        if constexpr (is_directed)
        {
            adj_.resize(vertex_count_ * vertex_count_);
        }
        else
        {
            // For an undirected graph, we only need to store the top triangular
            // section of the matrix because A[u][v] == A[v][u]. Thus, we can
            // use almost half the memory by not storing these duplicate values.
            adj_.resize((vertex_count_ * (vertex_count_ + 1)) / 2);
        }
    }

private:

    // Flattened matrix representation. We map 2D -> 1D internally.
    std::vector<EdgeID> adj_;
    size_t vertex_count_;
};

};
