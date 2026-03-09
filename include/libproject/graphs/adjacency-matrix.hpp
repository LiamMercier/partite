#pragma once
#include <vector>

#include <libproject/graphs/graph-constants.hpp>

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
private:

    // Flattened matrix representation. We must map 2D -> 1D internally.
    std::vector<EdgeID> adj_;
};
