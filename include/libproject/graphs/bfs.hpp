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

    enum class BFSAction
    {
        ContinueSearch,
        Skip,
        StopSearch
    };

    template <typename V>
    concept BFSVisitor = requires(V & visitor, VertexID u, VertexID v)
    {
        { visitor.source_vertex(u) };
        { visitor.discover_vertex(u) };
        { visitor.expand_vertex(u) };
        { visitor.examine_edge(u, v) };
        { visitor.tree_edge(u, v) };
        { visitor.non_tree_edge(u, v) };
        { visitor.finish_vertex(u) };
    };

    template <typename V>
    concept BFSDiscoverControl= requires (V & visitor, VertexID u)
    {
        { visitor.discover_vertex(u) } -> std::same_as<BFSAction>;
    };

    template <typename V>
    concept BFSExamineControl = requires (V & visitor, VertexID u, VertexID v)
    {
        { visitor.examine_edge(u, v) } -> std::same_as<BFSAction>;
    };

    //
    // BFS function bodies.
    //

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
        visitor.source_vertex(source);

        // If our visitor does vertex control flow, use
        // its early stopping or pruning rules.
        if constexpr (BFSDiscoverControl<V>)
        {
            BFSAction action = visitor.discover_vertex(source);

            if (action == BFSAction::Skip)
            {
                // Skipping here ends the traversal immediately
                // since there are no vertices in the queue.
                return;
            }
            else if (action == BFSAction::StopSearch)
            {
                return;
            }
        }
        // Otherwise, simply inform the visitor.
        else
        {
            visitor.discover_vertex(source);
        }

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
                // Before we classify an edge, tell our visitor.

                // If our visitor prunes or stops on edges, inform
                // it that we have found a new edge for it to do
                // early filtering on.
                if constexpr (BFSExamineControl<V>)
                {
                    BFSAction action = visitor.examine_edge(u, v);

                    if (action == BFSAction::Skip)
                    {
                        continue;
                    }
                    else if (action == BFSAction::StopSearch)
                    {
                        return;
                    }
                }
                // Otherwise, simply inform the visitor.
                else
                {
                    visitor.examine_edge(u, v);
                }

                // If we haven't seen this vertex
                if (seen[v.value] != visit_id)
                {
                    // Then, this is a tree edge.
                    visitor.tree_edge(u, v);

                    // Mark it as seen and push it into the queue.
                    seen[v.value] = visit_id;

                    // If our visitor does vertex control flow, use
                    // its early stopping or pruning rules.
                    if constexpr (BFSDiscoverControl<V>)
                    {
                        BFSAction action = visitor.discover_vertex(v);

                        if (action == BFSAction::Skip)
                        {
                            continue;
                        }
                        else if (action == BFSAction::StopSearch)
                        {
                            return;
                        }
                    }
                    // Otherwise, simply inform the visitor.
                    else
                    {
                        visitor.discover_vertex(v);
                    }

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
    // Visitor setups that can be reused.
    //

    struct NoOpBFSVisitor
    {
        void source_vertex(VertexID u)
        {}

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

    // The maximum distance in a graph is |V| - 1 so
    // setting Numeric = VertexID::underlying_type is a safe default.
    //
    // If a user knows distances are shorter, they can use a shorter type.
    template <typename Numeric = VertexID::underlying_type>
    struct DistanceBFSVisitor : NoOpBFSVisitor
    {
        using DistanceStorage = std::vector<Numeric>;

        DistanceBFSVisitor(std::vector<Numeric> & input_dists)
        :distances(input_dists)
        {}

        void tree_edge(VertexID u, VertexID v)
        {
            distances[v.value] = distances[u.value] + 1;
        }

        std::vector<Numeric> & distances;
    };

    // TODO: early stopping visitor

    //
    // BFS utility functions.
    //

    // For users who want to find shortest distances without
    // bothering with creating their own visitor.
    template <typename Graph, BFSWorkspaceCompatible Workspace>
    auto bfs_distances(const Graph & graph,
                       VertexID source,
                       Workspace & workspace)
    {
        using Visitor = DistanceBFSVisitor<VertexID::underlying_type>;

        // Allocate memory and give visitor a reference.
        size_t V_count = graph.vertex_count();
        Visitor::DistanceStorage distances(V_count);
        Visitor visitor(distances);

        bfs(graph, source, workspace, visitor);

        return std::move(distances);
    }

    // Temporary workspace overload.
    template <typename Graph>
    auto bfs_distances(const Graph & graph, VertexID source)
    {
        size_t V_count = graph.vertex_count();

        // Setup workspace for this epoch.
        BFSWorkspace workspace(V_count);
        workspace.queue.reserve(V_count);
        workspace.next();

        return std::move(bfs_distances(graph, source, workspace));
    }

};
