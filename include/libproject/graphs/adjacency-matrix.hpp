#pragma once
#include <vector>
#include <utility>
#include <iterator>
#include <ranges>
#include <cstddef>

#include <libproject/internal/libproject-assert.hpp>
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
        if constexpr (Directed)
        {
            adj_.resize(vertex_count_ * vertex_count_,
                        EdgeID::empty);

            in_deg_.resize(vertex_count_);
            out_deg_.resize(vertex_count_);
        }
        else
        {
            // For an undirected graph, we only need to store the top triangular
            // section of the matrix because A[u][v] == A[v][u]. Thus, we can
            // use almost half the memory by not storing these duplicate values.
            adj_.resize((vertex_count_ * (vertex_count_ + 1)) / 2,
                        EdgeID::empty);

            deg_.resize(vertex_count_);
        }
    }

    //
    // Iteration front end
    //

    auto neighbors(VertexID v) const
    {
        if constexpr(Directed)
        {
            return out_neighbors(v);
        }
        else
        {
            // We wish to search all edges (v, 0), ..., (v, |V|)
            //
            // index(v, v), ..., index(v, |V|) is stored in row v
            //
            // For i < v we know index(v, v - i) = index(v - i, v)
            // is stored in colum v of row i
            //
            // Thus the search is an L shape through the matrix.
            //
            // ? ? 0 ? ?
            //   ? 1 ? ?
            //     2 3 4
            //       ? ?
            //         ?

            // To do this without unnecessary multiplications, we use a
            // custom iterator and view that keeps track of the
            // current index and adds the correct offset.
            //
            // Then, map the index into array elements.
            //
            // Then, filter elements that represent empty edges.
            return NeighborsIndexView(v.value, vertex_count_)
                | std::views::transform([this](int indx){
                        return adj_[indx];
                    })
                | std::views::filter([](const auto & e_id){
                    return e_id != EdgeID::empty;
                });
        }
    }

    auto in_neighbors(VertexID v) const
        requires (Directed)
    {
        // For a digraph's incoming edges, we want all index values
        // corresponding to column v. We can use the transformation:
        //
        // {0, ..., |V| - 1} -> {v, ..., v + |V| * i} where 0 <= i < |V|
        return std::views::iota(size_t{0}, vertex_count_)
                | std::views::transform(
                    [this, v_0 = v.value](int indx)
                    {
                        return adj_[v_0 + (vertex_count_ * indx)];
                    })
                | std::views::filter([](const auto & e_id)
                    {
                        return e_id != EdgeID::empty;
                    });
    }

    auto out_neighbors(VertexID v) const
            requires (Directed)
    {
        // For a digraph's outgoing edges, we want all index values
        // corresponding to row v. We can use the transformation:
        //
        // {0, ..., |V| - 1} -> {v * |V|, ..., v * |V| + (|V| - 1)}
        return std::views::iota(size_t{0}, vertex_count_)
                | std::views::transform(
                    [this, v_0 = (v.value * vertex_count_)](int indx)
                    {
                        return adj_[v_0 + indx];
                    })
                | std::views::filter([](const auto & e_id)
                    {
                        return e_id != EdgeID::empty;
                    });
    }

    // Iterator over every edge in the adjacency list.
    auto edges() const
    {
        return std::views::iota(size_t{0}, adj_.size())
               | std::views::transform([this](int indx){
                        return adj_[indx];
                    })
               | std::views::filter([](const auto & e_id){
                        return e_id != EdgeID::empty;
                    });
    }

    //
    // Edge lookup and modification functions
    //

    EdgeID find_edge(VertexID from, VertexID to) const noexcept
    {
        size_t index = compute_index(from, to);

        libproject_assert(index < adj_.size(),
                          "AdjacencyMatrix::find_edge: index "
                          + std::to_string(index)
                          + " is out of bounds for matrix of "
                          + std::to_string(adj_.size())
                          + " elements.\nInput: {from: "
                          + std::to_string(from.value)
                          + ", to: "
                          + std::to_string(to.value)
                          +"}");

        return adj_[index];
    }

    EdgeID find_edge(std::pair<VertexID, VertexID> edge) const noexcept
    {
        return find_edge(edge.first, edge.second);
    }

    bool has_edge(VertexID from, VertexID to) const noexcept
    {
        size_t index = compute_index(from, to);

        libproject_assert(index < adj_.size(),
                          "AdjacencyMatrix::has_edge: index "
                          + std::to_string(index)
                          + " is out of bounds for matrix of "
                          + std::to_string(adj_.size())
                          + " elements.\nInput: {from: "
                          + std::to_string(from.value)
                          + ", to: "
                          + std::to_string(to.value)
                          +"}");

        EdgeID edge_id = adj_[index];
        return (edge_id != EdgeID::empty);
    }

    bool has_edge(std::pair<VertexID, VertexID> edge) const noexcept
    {
        return has_edge(edge.first, edge.second);
    }

    // Add an edge to the adjacency matrix.
    //
    // On success, return the EdgeID we supplied.
    // On failure, return the EdgeID of the already existing edge.
    EdgeID add_edge(VertexID from,
                    VertexID to,
                    EdgeID edge_id) noexcept
    {
        size_t index = compute_index(from, to);

        libproject_assert(index < adj_.size(),
                          "AdjacencyMatrix::add_edge: index "
                          + std::to_string(index)
                          + " is out of bounds for matrix of "
                          + std::to_string(adj_.size())
                          + " elements.\nInput: {from: "
                          + std::to_string(from.value)
                          + ", to: "
                          + std::to_string(to.value)
                          +"}");

        // Prevent double counting degrees.
        if (adj_[index] != EdgeID::empty)
        {
            return adj_[index];
        }

        adj_[index] = edge_id;

        if constexpr (!Directed)
        {
            libproject_assert(from.value < deg_.size(),
                              "AdjacencyMatrix::add_edge: index "
                              + std::to_string(from.value)
                              + " is out of bounds for degree list of "
                              + std::to_string(deg_.size())
                              + " elements (usually |V|)");

            libproject_assert(to.value < deg_.size(),
                              "AdjacencyMatrix::add_edge: index "
                              + std::to_string(to.value)
                              + " is out of bounds for degree list of "
                              + std::to_string(deg_.size())
                              + " elements (usually |V|)");

            deg_[from.value] += 1;
            deg_[to.value] += 1;
        }
        else
        {
            libproject_assert(to.value < in_deg_.size(),
                              "AdjacencyMatrix::add_edge: index "
                              + std::to_string(to.value)
                              + " is out of bounds for in degree list of "
                              + std::to_string(in_deg_.size())
                              + " elements (usually |V|)");

            libproject_assert(from.value < out_deg_.size(),
                              "AdjacencyMatrix::add_edge: index "
                              + std::to_string(from.value)
                              + " is out of bounds for out degree list of "
                              + std::to_string(out_deg_.size())
                              + " elements (usually |V|)");

            in_deg_[to.value] += 1;
            out_deg_[from.value] += 1;
        }

        return adj_[index];
    }

    [[maybe_unused]] EdgeID add_edge(std::pair<VertexID, VertexID> edge,
                                     EdgeID edge_id) noexcept
    {
        return add_edge(edge.first, edge.second, edge_id);
    }

    [[maybe_unused]] EdgeID remove_edge(VertexID from, VertexID to) noexcept
    {
        size_t index = compute_index(from, to);

        libproject_assert(index < adj_.size(),
                          "AdjacencyMatrix::remove_edge: index "
                          + std::to_string(index)
                          + " is out of bounds for matrix of "
                          + std::to_string(adj_.size())
                          + " elements.\nInput: {from: "
                          + std::to_string(from.value)
                          + ", to: "
                          + std::to_string(to.value)
                          +"}");

        EdgeID old_edge = adj_[index];

        // Prevent reducing the degree count for empty removal.
        if (old_edge == EdgeID::empty)
        {
            return old_edge;
        }

        adj_[index] = EdgeID::empty;

        if constexpr (!Directed)
        {
            libproject_assert(from.value < deg_.size(),
                              "AdjacencyMatrix::remove_edge: index "
                              + std::to_string(from.value)
                              + " is out of bounds for degree list of "
                              + std::to_string(deg_.size())
                              + " elements (usually |V|)");

            libproject_assert(to.value < deg_.size(),
                              "AdjacencyMatrix::remove_edge: index "
                              + std::to_string(to.value)
                              + " is out of bounds for degree list of "
                              + std::to_string(deg_.size())
                              + " elements (usually |V|)");

            deg_[from.value] -= 1;
            deg_[to.value] -= 1;
        }
        else
        {
            libproject_assert(to.value < in_deg_.size(),
                              "AdjacencyMatrix::remove_edge: index "
                              + std::to_string(to.value)
                              + " is out of bounds for in degree list of "
                              + std::to_string(in_deg_.size())
                              + " elements (usually |V|)");

            libproject_assert(from.value < out_deg_.size(),
                              "AdjacencyMatrix::remove_edge: index "
                              + std::to_string(from.value)
                              + " is out of bounds for out degree list of "
                              + std::to_string(out_deg_.size())
                              + " elements (usually |V|)");

            in_deg_[to.value] -= 1;
            out_deg_[from.value] -= 1;
        }

        return old_edge;
    }

    [[maybe_unused]] EdgeID remove_edge(std::pair<VertexID,
                                                  VertexID> edge) noexcept
    {
        return remove_edge(edge.first, edge.second);
    }

    // Clear all edges
    void clear() noexcept
    {
        // Set edges to empty.
        for (EdgeID & edge : adj_)
        {
            edge.value = EdgeID::empty;
        }

        // Set degree to zero for all vertices.
        if constexpr (!Directed)
        {
            libproject_assert(deg_.size() == vertex_count_,
                              "AdjacencyMatrix::clear: degree list size "
                              "does not equal vertex count\n|V|: "
                              + std::to_string(vertex_count_)
                              + "\nDegree list size: "
                              + std::to_string(deg_.size()));

            for (size_t i = 0; i < vertex_count_; i++)
            {
                deg_[i] = 0;
            }
        }
        else
        {
            libproject_assert(in_deg_.size() == vertex_count_,
                              "AdjacencyMatrix::clear: indegree list size "
                              "does not equal vertex count\n|V|: "
                              + std::to_string(vertex_count_)
                              + "\nDegree list size: "
                              + std::to_string(in_deg_.size()));

            libproject_assert(out_deg_.size() == vertex_count_,
                              "AdjacencyMatrix::clear: outdegree list size "
                              "does not equal vertex count\n|V|: "
                              + std::to_string(vertex_count_)
                              + "\nDegree list size: "
                              + std::to_string(out_deg_.size()));

            for (size_t i = 0; i < vertex_count_; i++)
            {
                in_deg_[i] = 0;
                out_deg_[i] = 0;
            }
        }
    }

    //
    // Utility functions
    //

    size_t degree(VertexID v) const noexcept
    {
        if constexpr (!Directed)
        {
            return deg_[v.value];
        }
        else
        {
            return in_degree(v) + out_degree(v);
        }
    }

    size_t in_degree(VertexID v) const noexcept
        requires (Directed)
    {
        return in_deg_[v.value];
    }

    size_t out_degree(VertexID v) const noexcept
        requires (Directed)
    {
        return out_deg_[v.value];
    }

    bool has_self_loops() const noexcept
    {
        if constexpr (Directed)
        {
            size_t index = 0;

            for (size_t i = 0; i < vertex_count_; i++)
            {
                if (adj_[index] != EdgeID::empty)
                {
                    return true;
                }

                index += vertex_count_ + 1;
            }

            return false;
        }
        else
        {
            size_t index = 0;
            for (size_t i = 0; i < vertex_count_; i++)
            {
                if (adj_[index] != EdgeID::empty)
                {
                    return true;
                }

                index += vertex_count_ - i;
            }

            return false;
        }
    }

    bool has_self_loop(VertexID v) const noexcept
    {
        return has_edge(v, v);
    }

    bool valid_vertex(VertexID v) const noexcept
    {
        return v.value < vertex_count_;
    }

    size_t vertex_count() const noexcept
    {
        return vertex_count_;
    }

    size_t size() const noexcept
    {
        return vertex_count();
    }

    //
    // Transformations
    //

    // TODO <optimization>: consider a compute_transpose overload that
    //                      takes a list (view?) of edges to check from
    //                      the graph container's EdgeArrays to/from vectors.

    [[nodiscard]] AdjacencyMatrix compute_transpose() const
    {
        if constexpr (Directed)
        {
            // Construct a new, empty matrix.
            AdjacencyMatrix rev(vertex_count_);

            // Go over each edge (v, u) and insert (u, v)
            for (size_t v = 0; v < vertex_count_; v++)
            {
                // TODO <optimization>: we could stop early when we
                //                      have found out_deg(v) elements.
                for (size_t u = 0; u < vertex_count_; u++)
                {
                    EdgeID e_id = find_edge(VertexID(v), VertexID(u));

                    if (e_id != EdgeID::empty)
                    {
                        rev.add_edge(u, v, e_id);
                    }
                }
            }

            return rev;
        }
        // The transpose of an undirected graph is the same graph.
        else
        {
            return AdjacencyMatrix {
                adj_,
                vertex_count_,
                deg_
            };
        }

    }

private:
    size_t compute_index(VertexID from, VertexID to) const noexcept
    {
        if constexpr (Directed)
        {
            return from.value * vertex_count_ + to.value;
        }
        else
        {
            if (from > to)
            {
                std::swap(from, to);
            }

            // let "from" be u, and "to" be v
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

//
// Custom view for internal use in AdjacencyMatrix
//

private:
    // Adjacency matrix based custom view to reduce multiplications when
    // traversing the flat array.
    //
    // The increment logic is based on the following proof of concept,
    // but translated so we can return a view.
    //
    //     // We wish to find all edges (v, 0), ..., (v, |V|)
    //     //
    //     // index(v, v), ..., index(v, |V|) is stored in row v
    //     //
    //     // index(v, v - i) = index(v - i, v) is stored in row i
    //     //
    //     // Thus, instead of computing index multiple times, we can compute
    //     // one index and increase / decrease offsets.
    //     //
    //     // We know that row i has exactly i elements missing, thus for i < v
    //     //
    //     // index(v, (i + 1)) - index(v, i) = |V| - (i + 1)
    //     //
    //     // So, start at index(v, 0) = index(0, v) = v and then for i = 1 to v
    //     // increase the value by |V| - i
    //     //
    //     // Then, for j = v + 1 to |V| increase the value by 1
    //
    //     // we need to remove this vector and make a view later
    //     std::vector<size_t> n;
    //
    //     // start at index(0, v)
    //     size_t index = v;
    //     n.push_back(index);
    //
    //     // for i = 1 to v increase the value by |V| - i
    //     for (size_t i = 1; i <= v; i++)
    //     {
    //         index += (vertex_count_ - i);
    //         n.push_back(index);
    //     }
    //
    //     // for j = v + 1 to |V| increase the value by 1
    //     for (size_t j = v + 1; j < vertex_count_; j++)
    //     {
    //         index += 1;
    //         n.push_back(index);
    //     }
    //
    //     return n;
    class NeighborsIndexView
    {
    public:
        NeighborsIndexView(size_t v, size_t vertex_count)
        :v_(v),
        vertex_count_(vertex_count)
        {}

        class Iterator
        {
            public:
                using iterator_concept = std::forward_iterator_tag;
                using value_type = size_t;
                // (size_t - size_t) specific type.
                using difference_type = std::ptrdiff_t;

                Iterator() = default;

                // Start at index = v
                Iterator(size_t v, size_t V)
                :v_(v),
                V_(V),
                step_(0),
                current_index_(v)
                {}

                // End at step = |V| (at which point index = compute_index(v, |V|))
                Iterator(size_t v, size_t V, size_t step)
                :v_(v),
                V_(V),
                step_(step),
                current_index_(0)
                {}

                // Iterator value is just the current array index
                size_t operator*() const
                {
                    return current_index_;
                }

                // Increment based on the rules in the above loops
                Iterator & operator++()
                {
                    step_++;

                    if(step_ <= v_)
                    {
                        current_index_ += (V_ - step_);
                    }
                    else
                    {
                        current_index_ += 1;
                    }

                    return *this;
                }

                // forced to do this to satisfy forward_iterator_tag
                //
                // do not use
                Iterator operator++(int)
                {
                    Iterator tmp = *this;
                    ++(*this);
                    return tmp;
                }

                // we only compare against .end() so we just want to have done each step.
                bool operator==(const Iterator & other) const
                {
                    return step_ == other.step_;
                }

            private:
                size_t v_{0};
                size_t V_{0};
                size_t step_{0};
                size_t current_index_{0};
        };

        Iterator begin() const
        {
            return Iterator(v_, vertex_count_);
        }

        Iterator end() const
        {
            return Iterator{v_, vertex_count_, vertex_count_};
        }

    // v is the vertex we are computing for, vertex_count_ is |V|
    private:
        size_t v_;
        size_t vertex_count_;
    };

private:
    // Internal constructors for returning a copy.
    AdjacencyMatrix(std::vector<EdgeID> adj,
                    size_t v_count,
                    std::vector<size_t> deg)
        requires (!Directed)
    :adj_(std::move(adj)),
    vertex_count_(v_count),
    deg_(std::move(deg))
    {
    }

//
// Adjacency Matrix private data members.
//

private:

    // Flattened matrix representation. We map 2D -> 1D internally.
    std::vector<EdgeID> adj_;
    size_t vertex_count_;

    // Mapping from VertexID -> size_t (vertex degree)
    //
    // For undirected graphs, we only need one vector. Two for digraphs.
    std::conditional_t<!Directed, std::vector<size_t>, std::monostate> deg_;
    std::conditional_t<Directed, std::vector<size_t>, std::monostate> in_deg_;
    std::conditional_t<Directed, std::vector<size_t>, std::monostate> out_deg_;
};

};
