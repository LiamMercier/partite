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
    template <typename SeenType>
    struct BFSWorkspace
    {
        std::vector<SeenType> seen;
        std::vector<VertexID> queue;

        // Generation based IDs so we can avoid overwriting the seen
        // set every time we use the workspace again.
        SeenType visit_id{0};

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

    template <typename Graph>
    void bfs(const Graph & graph, VertexID source)
    {
        // we will do this later, just create a temporary workspace
        // and call the other function for users who do not care about
        // managing or reusing their own memory.
    }

    // BFS for standard graphs.
    template <typename Graph, typename Workspace>
    void bfs(const Graph & graph,
             VertexID source,
             Workspace & workspace)
    {
        auto & seen = workspace.seen;
        auto & queue = workspace.queue;
        const auto visit_id = workspace.visit_id;

        // Track the start of the queue.
        size_t head = 0;

        // Insert source into the queue.
        queue.push_back(source);

        // Mark this vertex as seen.
        seen[source.value] = visit_id;

        // While the queue is not empty (head is not at the end)
        while (head < queue.size())
        {
            // Pop the next vertex by advancing the head index.
            VertexID u = queue[head];
            head++;

            // Iterate over each neighbor
            for (const auto & v : graph.neighbors(u))
            {
                // If we haven't seen this vertex
                if (seen[v.value] != visit_id)
                {
                    // Mark it as seen and push it into the queue.
                    seen[v.value] = visit_id;
                    queue.push_back(v);
                }
            }
        }
    }
};
