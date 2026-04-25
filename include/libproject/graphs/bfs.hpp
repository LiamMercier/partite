#pragma once

#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>

namespace libproject::graphs
{
    // Optional struct for reusing memory across BFS calls.
    //
    // We use a vector for the seen set to track
    // which vertices have been used, and a vector
    // that grows as we push to it as our queue.
    //
    // While the queue is going to waste space (we never
    // reduce the size when we pop a vertex), it should
    // be fine as we already use Theta(|V|) memory for
    // the seen set, and we do not have to pay the cost
    // of reallocating or reordering vertices on removal.
    template <typename SeenType = uint16_t>
    struct BFSWorkspace
    {
        using seen_type = SeenType;

        BFSWorkspace(size_t num_vertices)
        :seen(num_vertices)
        {}

        std::vector<SeenType> seen;
        std::vector<VertexID> queue;

        // Generation based IDs so we can avoid overwriting the seen
        // set every time we use the workspace again.
        seen_type visit_id{0};

        void next()
        {
            visit_id++;

            // Clear if we ever wrap around.
            if (visit_id == 0)
            {
                std::fill(seen.begin(), seen.end(), 0);
                visit_id = 1;
            }

            queue.clear();
        }
    };

    template <typename W>
    concept BFSWorkspaceCompatible = requires (W & w)
    {
        typename W::seen_type;
        { w.seen };
        { w.queue };
        { w.visit_id };
        { w.next() };
    };

    template <typename V>
    concept BFSVisitor = requires(V & visitor, VertexID u, VertexID v)
    {
        { visitor.discover_vertex(u) };
        { visitor.expand_vertex(u) };
        { visitor.examine_edge(u, v) };
        { visitor.tree_edge(u, v) };
        { visitor.non_tree_edge(u, v) };
        { visitor.finish_vertex(u) };
    };

    // BFS for standard graphs.
    template <typename Graph,
              BFSWorkspaceCompatible Workspace,
              BFSVisitor V>
    void bfs(const Graph & graph,
             VertexID source,
             Workspace & workspace,
             V&& visitor)
    {
        auto & seen = workspace.seen;
        auto & queue = workspace.queue;
        const auto visit_id = workspace.visit_id;

        // Track the start of the queue.
        size_t head = 0;

        // Push the source.
        seen[source.value] = visit_id;
        visitor.discover_vertex(source);
        queue.push_back(source);

        // While the queue is not empty (head is not at the end)
        while (head < queue.size())
        {
            // Pop the next vertex by advancing the head index.
            VertexID u = queue[head];
            head++;

            visitor.expand_vertex(u);

            for (const VertexID & v : graph.neighbors(u))
            {
                // Before we classify an edge, notify.
                visitor.examine_edge(u, v);

                // If we haven't seen this vertex
                if (seen[v.value] != visit_id)
                {
                    // Then, this is a tree edge.
                    visitor.tree_edge(u, v);

                    // Mark it as seen and push it into the queue.
                    seen[v.value] = visit_id;
                    visitor.discover_vertex(v);
                    queue.push_back(v);
                }
                else
                {
                    // Otherwise, v is already discovered, so this
                    // was not a tree edge.
                    visitor.non_tree_edge(u, v);
                }
            }

            visitor.finish_vertex(u);
        }
    }

    // Overload that creates a temporary workspace for users who
    // do not care about reusing memory.
    template <typename Graph, BFSVisitor V>
    void bfs(const Graph & graph, VertexID source, V&& visitor)
    {
        BFSWorkspace workspace(graph.vertex_count());
        workspace.queue.reserve(graph.vertex_count());
        workspace.next();

        bfs(graph, source, workspace, std::forward<V>(visitor));
    }

    //
    // Some visitor setups that can be reused.
    //

    struct NoOpBFSVisitor
    {
        void discover_vertex(VertexID u)
        {}

        void expand_vertex(VertexID u)
        {}

        void examine_edge(VertexID u, VertexID v)
        {}

        void tree_edge(VertexID u, VertexID v)
        {}

        void non_tree_edge(VertexID u, VertexID v)
        {}

        void finish_vertex(VertexID u)
        {}
    };

    // The maximum distance in a graph is |V| - 1 so this is a safe default.
    template <typename Numeric = VertexID::underlying_type>
    struct DistanceBFSVisitor : NoOpBFSVisitor
    {
        DistanceBFSVisitor(std::vector<Numeric> & input_dists)
        :distances(input_dists)
        {}

        void tree_edge(VertexID u, VertexID v)
        {
            distances[v.value] = distances[u.value] + 1;
        }

        std::vector<Numeric> & distances;
    };
};
