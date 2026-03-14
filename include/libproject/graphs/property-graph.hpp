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

    template<typename... Args>
    bool add_edge(VertexID from, VertexID to, Args&&... args)
        requires (std::is_same_v<EdgeData, std::monostate>
                  && sizeof...(Args) == 0) ||
                 (!std::is_same_v<EdgeData, std::monostate>
                  && std::constructible_from<EdgeData, Args...>)
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

            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                edges_.data[e_id.value] = EdgeData{std::forward<Args>(args)...};
            }

            edges_.from[e_id.value] = from;
            edges_.to[e_id.value] = to;
        }
        // Otherwise, entire vector is full, add a new edge.
        else
        {
            e_id = edges_.size();

            if constexpr (!std::is_same_v<EdgeData, std::monostate>)
            {
                (edges_.data).emplace_back(std::forward<Args>(args)...);
            }

            (edges_.from).push_back(from);
            (edges_.to).push_back(to);

            bool res = representation_.add_edge(from, to, e_id);

            // Prevent bad modifications.
            if (!res)
            {
                if constexpr (!std::is_same_v<EdgeData, std::monostate>)
                {
                    (edges_.data).pop_back();
                }

                (edges_.from).pop_back();
                (edges_.to).pop_back();
                return res;
            }
        }

        return true;
    }

    // std::pair overloads.
    bool add_edge(std::pair<VertexID, VertexID> edge, EdgeData data)
        requires (!std::is_same_v<EdgeData, std::monostate>)
    {
        return add_edge(edge.first, edge.second, std::move(data));
    }

    bool add_edge(std::pair<VertexID, VertexID> edge)
        requires (std::is_same_v<EdgeData, std::monostate>)
    {
        return add_edge(edge.first, edge.second);
    }

    // Remove an edge, this will have to invalidate any EdgeData reference.
    [[maybe_unused]] bool remove_edge(VertexID from, VertexID to)
    {
        EdgeID res = representation_.remove_edge(from, to);

        if (res == EdgeID::empty)
        {
            return false;
        }

        // If edge was not empty, remove from the list.
        free_edges_.push_back(res);

        edges_.from[res.value] = VertexID::empty;
        edges_.to[res.value] = VertexID::empty;

        return true;
    }

    [[maybe_unused]] bool remove_edge(std::pair<VertexID, VertexID> edge)
    {
        return remove_edge(edge.first, edge.second);
    }

    // Remove and return an edge's data. EdgeData must not be monostate.
    bool try_pop_edge(VertexID from, VertexID to, EdgeData & out)
        requires (!std::is_same_v<EdgeData, std::monostate>)
    {
        EdgeID res = representation_.remove_edge(from, to);

        if (res == EdgeID::empty)
        {
            return false;
        }

        // If edge was not empty, remove from the list.
        free_edges_.push_back(res);

        // Overwrite with default data.
        out = std::move(edges_.data[res.value]);

        edges_.data[res.value] = {};
        edges_.from[res.value] = VertexID::empty;
        edges_.to[res.value] = VertexID::empty;

        return true;
    }

    bool try_pop_edge(std::pair<VertexID, VertexID> edge, EdgeData & out)
        requires (!std::is_same_v<EdgeData, std::monostate>)
    {
        return try_pop_edge(edge.first, edge.second, out);
    }

    // TODO: maybe give overload like std::optional<EdgeData> pop_edge(...)
    //
    // TODO: maybe also provide erase_edge() for explicit immediate overwrites

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

        size_t size() const noexcept
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
