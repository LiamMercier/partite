#pragma once
#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{

// TODO <feature>: asserts for the transpose view instead of
//                 using the property graph asserts.

// A lightweight view of a graph as its transpose.
//
// By definition, E^T = {(u, v) | (v, u) \in E}.
//
// We can take advantage of this for lookup calls
// since we can already query (v, u) and thus any
// query for (u, v) can be resolved by swapping
// the order of arguments.
//
// For iteration, we can reverse inward methods
// with outward methods and visa versa.
template <typename Graph>
class TransposeView
{
public:
    using EdgePair = typename Graph::EdgePair;

    static constexpr bool directed = Graph::directed;

public:
    explicit TransposeView(const Graph & graph) noexcept
    :graph_(&graph)
    {}

// Transposed operations
public:
    decltype(auto) find_edge(VertexID from, VertexID to) const
    {
        return graph_->find_edge(to, from);
    }

    decltype(auto) endpoints(EdgeID e_id) const noexcept
    {
        auto ep = graph_->endpoints(e_id);

        // Swap (source, target)
        return EdgePair{ep.second, ep.first};
    }

    decltype(auto) source(EdgeID e_id) const noexcept
    {
        return graph_->target(e_id);
    }

    decltype(auto) target(EdgeID e_id) const noexcept
    {
        return graph_->source(e_id);
    }

    decltype(auto) has_edge(VertexID from, VertexID to) const
    {
        return graph_->has_edge(to, from);
    }

    decltype(auto) edge_view(EdgeID e_id) const
    {
        auto ev = graph_->edge_view(e_id);

        // Swap (source, target) and keep the same ID
        return EdgeView{ev.to, ev.from, ev.id};
    }

    // We must swap from and to here as well.
    decltype(auto) edges() const
    {
        return graph_->edges()
               | std::views::transform([this](EdgeView ev){
                       return EdgeView{ev.to,
                                       ev.from,
                                       ev.id};
                   });
    }

    // Neighboring edges calls
    //
    // Undirected graphs are unchanged, we simply forward.
    // Directed graphs need outgoing swapped with incoming.

    decltype(auto) neighbors(VertexID v) const
        requires (!directed)
    {
        return graph_->neighbors(v);
    }

    decltype(auto) neighbors(VertexID v) const
        requires (directed)
    {
        return out_neighbors(v);
    }

    decltype(auto) out_neighbors(VertexID v) const
        requires (directed) &&
                 requires (const Graph & g, VertexID v){ g.in_neighbors(v); }
    {
        return graph_->in_neighbors(v);
    }

    decltype(auto) in_neighbors(VertexID v) const
        requires (directed) &&
                 requires (const Graph & g, VertexID v){ g.out_neighbors(v); }
    {
        return graph_->out_neighbors(v);
    }

    decltype(auto) incident_edges(VertexID v) const
        requires (!directed)
    {
        return graph_->incident_edges(v);
    }

    decltype(auto) incident_edges(VertexID v) const
        requires (directed)
    {
        return out_edges(v);
    }

    decltype(auto) out_edges(VertexID v) const
        requires (directed) &&
                 requires (const Graph & g, VertexID v){ g.in_edges(v); }
    {
        return graph_->in_edges(v);
    }

    decltype(auto) in_edges(VertexID v) const
        requires (directed) &&
                 requires (const Graph & g, VertexID v){ g.out_edges(v); }
    {
        return graph_->out_edges(v);
    }

    decltype(auto) incident_edge_ids(VertexID v) const
        requires (!directed)
    {
        return graph_->incident_edge_ids(v);
    }

    decltype(auto) incident_edge_ids(VertexID v) const
        requires (directed)
    {
        return out_edge_ids(v);
    }

    decltype(auto) out_edge_ids(VertexID v) const
        requires (directed) &&
                 requires (const Graph & g, VertexID v){ g.in_edge_ids(v); }
    {
        return graph_->in_edge_ids(v);
    }

    decltype(auto) in_edge_ids(VertexID v) const
        requires (directed) &&
                 requires (const Graph & g, VertexID v){ g.out_edge_ids(v); }
    {
        return graph_->out_edge_ids(v);
    }

    decltype(auto) in_degree(VertexID v) const
        requires (directed) &&
                 requires (const Graph & g, VertexID v){ g.out_degree(v); }
    {
        return graph_->out_degree(v);
    }

    decltype(auto) out_degree(VertexID v) const
        requires (directed) &&
                 requires (const Graph & g, VertexID v){ g.in_degree(v); }
    {
        return graph_->in_degree(v);
    }

    //
    // Pair overloads for our transposed functions
    //

    decltype(auto) find_edge(EdgePair edge) const
    {
        return find_edge(edge.first, edge.second);
    }

    decltype(auto) has_edge(EdgePair edge) const
    {
        return has_edge(edge.first, edge.second);
    }

// Forwarded functions
public:
    decltype(auto) vertices() const
    {
        return graph_->vertices();
    }

    decltype(auto) edge_ids() const
    {
        return graph_->edge_ids();
    }

    decltype(auto) edge_data(EdgeID e_id) const noexcept
    {
        return graph_->edge_data(e_id);
    }

    decltype(auto) vertex_data(VertexID v) const noexcept
    {
        return graph_->vertex_data(v);
    }

    decltype(auto) vertex_count() const noexcept
    {
        return graph_->vertex_count();
    }

    decltype(auto) edge_count() const noexcept
    {
        return graph_->edge_count();
    }

    decltype(auto) degree(VertexID v) const
    {
        return graph_->degree(v);
    }

    decltype(auto) operator[](VertexID v) const noexcept
    {
        return vertex_data(v);
    }

    decltype(auto) operator[](EdgeID e) const noexcept
    {
        return edge_data(e);
    }

    decltype(auto) has_self_loop(VertexID v) const
    {
        return graph_->has_self_loop(v);
    }

    decltype(auto) has_self_loops() const
    {
        return graph_->has_self_loops();
    }

    decltype(auto) valid_vertex(VertexID v) const
    {
        return graph_->valid_vertex(v);
    }

    decltype(auto) valid_edge(EdgeID e_id) const noexcept
    {
        return graph_->valid_edge(e_id);
    }

private:
    const Graph* graph_;
};

};
