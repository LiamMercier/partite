#pragma once
#include <vector>

#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{

template <bool Directed>
class AdjacencyMatrix
{
public:
    static constexpr bool is_directed = Directed;

    // Create an AdjacencyMatrix with all zero edges.
    AdjacencyMatrix(size_t num_vertices)
    :vertex_count_(num_vertices)
    {
        if constexpr (is_directed)
        {
            adj_.resize(vertex_count_ * vertex_count_,
                        EdgeID::empty);
        }
        else
        {
            // For an undirected graph, we only need to store the top triangular
            // section of the matrix because A[u][v] == A[v][u]. Thus, we can
            // use almost half the memory by not storing these duplicate values.
            adj_.resize((vertex_count_ * (vertex_count_ + 1)) / 2,
                        EdgeID::empty);
        }
    }

    //
    // Edge lookup and modification functions
    //

    EdgeID find_edge(VertexID from, VertexID to) const
    {
        size_t index = compute_index(from, to);
        return adj_[index];
    }

    bool has_edge(VertexID from, VertexID to) const
    {
        size_t index = compute_index(from, to);
        EdgeID edge_id = adj_[index];

        return (edge_id != EdgeID::empty);
    }

    void add_edge(VertexID from, VertexID to, EdgeID edge_id)
    {
        size_t index = compute_index(from, to);
        adj_[index] = edge_id;
        return;
    }

    void remove_edge(VertexID from, VertexID to)
    {
        size_t index = compute_index(from, to);
        adj_[index] = VertexID::empty;
        return;
    }

    //
    // Utility functions
    //

    size_t vertex_count() const noexcept
    {
        return vertex_count_;
    }

    size_t size() const noexcept
    {
        return vertex_count();
    }

private:
    size_t compute_index(VertexID from, VertexID to)
    {
        if constexpr (is_directed)
        {
            return from.value * vertex_count_ + to.value;
        }
        else
        {
            if (from > to)
            {
                std::swap(from, to);
            }

            // let from be u, and to be v
            //
            // row i has |V| - i elements, so the elements before row u is
            //
            // \sum_{i=0}^{u-1}(|V|-i)
            //      = u|V| - ((u - 1) * u) / 2
            //
            // the offset inside of this row is of course, (v - u)
            //
            // combining terms, the index is
            //
            // u|V| + v - ((u + 1) * u) / 2
            return from.value * vertex_count_
                   + to.value
                   - ((from.value + 1) * from.value) / 2;
        }
    }

private:

    // Flattened matrix representation. We map 2D -> 1D internally.
    std::vector<EdgeID> adj_;
    size_t vertex_count_;
};

};
