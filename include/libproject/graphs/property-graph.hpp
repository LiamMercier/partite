#pragma once

#include <variant>

#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>

namespace libproject::graphs
{

// TODO: debug mode asserts
template <GraphRepresentation Representation,
          typename VertexData = std::monostate,
          typename EdgeData = std::monostate>
class PropertyGraph
{
public:
    // TODO: do we actually want this? Decide later.
    struct EdgeView
    {
        VertexID from;
        VertexID to;
        EdgeData* data;
    };

public:
    static constexpr bool is_directed = Representation::is_directed;

    // Empty graph constructor
    PropertyGraph(size_t num_vertices)
    :representation_(num_vertices),
    vertices_(num_vertices)
    {
    }

    //
    // Edge operations
    //

    bool has_edge(VertexID from, VertexID to) const
    {
        return representation_.has_edge(from, to);
    }

    bool has_edge(std::pair<VertexID, VertexID> edge) const
    {
        return has_edge(edge.first, edge.second);
    }

    // For templates with edge data.
    bool add_edge(VertexID from, VertexID to, EdgeData data)
        requires (!std::is_same_v<EdgeData, std::monostate>)
    {
        EdgeID e_id;

        // If we have deleted an edge in the past, use the old slot.
        if (!free_edges_.empty())
        {
            e_id = free_edges_.back();
            free_edges_.pop_back();

            bool res = representation_.add_edge(from, to, e_id);

            // Prevent bad modifications.
            if (!res)
            {
                free_edges_.push_back(e_id);
                return res;
            }

            edges_.data[e_id.value] = std::move(data);
            edges_.from[e_id.value] = from;
            edges_.to[e_id.value] = to;
        }
        // Otherwise, entire vector is full, add a new edge.
        else
        {
            e_id = edges_.size();

            bool res = representation_.add_edge(from, to, e_id);

            // Prevent bad modifications.
            if (!res)
            {
                return res;
            }

            (edges_.data).push_back(data);
            (edges_.from).push_back(from);
            (edges_.to).push_back(to);
        }

        return true;
    }

    // For templates without edge data
    bool add_edge(VertexID from, VertexID to)
        requires (std::is_same_v<EdgeData, std::monostate>)
    {
        EdgeID e_id;

        if (!free_edges_.empty())
        {
            e_id = free_edges_.back();
            free_edges_.pop_back();

            bool res = representation_.add_edge(from, to, e_id);

            // Prevent bad modifications.
            if (!res)
            {
                free_edges_.push_back(e_id);
                return res;
            }

            edges_.from[e_id.value] = from;
            edges_.to[e_id.value] = to;
        }
        else
        {
            e_id = (edges_.to).size();

            bool res = representation_.add_edge(from, to, e_id);

            if (!res)
            {
                return res;
            }

            (edges_.from).push_back(from);
            (edges_.to).push_back(to);
        }

        return true;
    }

    // std::pair overloads.
    bool add_edge(std::pair<VertexID, VertexID> edge, EdgeData data)
        requires (!std::is_same_v<EdgeData, std::monostate>)
    {
        return add_edge(edge.first, edge.second, data);
    }

    bool add_edge(std::pair<VertexID, VertexID> edge)
        requires (std::is_same_v<EdgeData, std::monostate>)
    {
        return add_edge(edge.first, edge.second);
    }

    [[maybe_unused]] bool remove_edge(VertexID from, VertexID to)
    {
        EdgeID res = representation_.remove_edge(from, to);

        if (res == EdgeID::empty)
        {
            return false;
        }

        // If edge was not empty, remove from the list.
        free_edges_.push_back(res);

        // Overwrite with default data.
        if constexpr (!std::is_same_v<EdgeData, std::monostate>)
        {
            edges_.data[res] = EdgeData();
        }

        return true;
    }

    [[maybe_unused]] bool remove_edge(std::pair<VertexID, VertexID> edge)
    {
        return remove_edge(edge.first, edge.second);
    }

    // TODO <feature>: consider EdgeData pop_edge(from, to) or similar?

    //
    // Transformations
    //

    // Compute a new graph where the edges are reversed.
    [[nodiscard]] PropertyGraph compute_transpose() const
    {
        Representation rev = representation_.compute_transpose();

        return PropertyGraph {
            std::move(rev),
            vertices_,
            edges_,
            free_edges_
        };
    }

    // TODO: transpose view

private:
    // Internal structure-of-arrays storage.
    struct EdgeArrays
    {
        void resize(size_t N)
        {
            from.resize(N);
            to.resize(N);

            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                data.resize(N);
            }
        }

        size_t size()
        {
            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                return data.size();
            }
            else
            {
                return from.size();
            }
        }

        std::vector<VertexID> from;
        std::vector<VertexID> to;

        // Define "std::vector<EdgeData> data" if not monostate.
        std::conditional_t<
            std::is_same_v<EdgeData, std::monostate>,
            std::monostate,
            std::vector<EdgeData>> data;
    };

    // Internal constructor for returning a copy of the graph.
    PropertyGraph(Representation rep,
                  std::vector<VertexData> v,
                  EdgeArrays e,
                  std::vector<EdgeID> free)
    :representation_(std::move(rep)),
    vertices_(std::move(v)),
    edges_(std::move(e)),
    free_edges_(std::move(free))
    {
    }

private:
    Representation representation_;
    std::vector<VertexData> vertices_;

    // stores {from, to, data} entries.
    EdgeArrays edges_;
    // index values into edges_ where we have previously deleted an element.
    std::vector<EdgeID> free_edges_;
};

};
