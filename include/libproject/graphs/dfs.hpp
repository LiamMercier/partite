#pragma once

#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>
#include <libproject/graphs/search-visitor.hpp>

namespace libproject::graphs
{
    // Optional struct for reusing memory across DFS calls.
    struct DFSWorkspace
    {
        DFSWorkspace() = default;

        // Basic vector based stack structure.
        struct DFSWorkspaceStack
        {
            void push(VertexID u)
            {
                s_.push_back(u);
            }

            VertexID pop()
            {
                VertexID temp = s_.back();
                s_.pop_back();
                return temp;
            }

            size_t size() const
            {
                return s_.size();
            }

            void reserve(size_t N)
            {
                s_.reserve(N);
            }

            void clear()
            {
                s_.clear();
            }

            std::vector<VertexID> s_;
        };

        void next()
        {
            stack.clear();
        }

        DFSWorkspaceStack stack;
    };

    enum class DFSColorState : uint8_t
    {
        White,
        Grey,
        Black
    };

    // Workspace with a color array included.
    struct DFSColorWorkspace : DFSWorkspace
    {
        DFSColorWorkspace(size_t num_vertices)
        :colors(num_vertices)
        {}

        DFSColorState color(VertexID v)
        {
            return colors[v.value];
        }

        void set_color(VertexID v, DFSColorState state)
        {
            colors[v.value] = state;
        }

        std::vector<DFSColorState> colors;
    };

    template <typename W>
    concept DFSCompatibleWorkspace = requires (W & w, VertexID u)
    {
        { w.stack };
        { w.stack.push(u) };
        { w.stack.pop() } -> std::convertible_to<VertexID>;
        { w.next() };
    };

    // For checking if a workspace or visitor has a .color(u)
    // method to determine the state of a node.
    template <typename T>
    concept TracksColorState = requires (T & t, VertexID u)
    {
        { t.color(u) } -> std::convertible_to<DFSColorState>;
    };

    template <AdjacencyGraph Graph,
              DFSCompatibleWorkspace Workspace,
              SearchVisitor Visitor>
    void dfs(const Graph & graph,
             VertexID source,
             Workspace & workspace,
             Visitor&& visitor)
    {
        auto & stack = workspace.stack;

        visitor.source_vertex(source);

        if constexpr (DiscoverControllable<Visitor>)
        {
            ControlAction action = visitor.discover_vertex(source);

            if (action == ControlAction::Skip)
            {
                // Stack would be empty, return.
                return;
            }
            else if (action == ControlAction::Stop)
            {
                return;
            }
        }
        else
        {
            visitor.discover_vertex(source);
        }

        stack.push(source);

        // While the stack has vertices to visit.
        while (stack.size() > 0)
        {
            // Pop the next vertex
            VertexID u = stack.pop();
            visitor.expand_vertex(u);

            for (const VertexID & v : graph.neighbors(u))
            {
                // Apply any filtering rules.
                if constexpr (ExamineControllable<Visitor>)
                {
                    ControlAction action = visitor.examine_edge(u, v);

                    if (action == ControlAction::Skip)
                    {
                        continue;
                    }
                    else if (action == ControlAction::Stop)
                    {
                        return;
                    }
                }
                else
                {
                    visitor.examine_edge(u, v);
                }

                // Check if this vertex has been seen or not.
                bool seen = false;
                if constexpr (TracksColorState<Visitor>)
                {
                    seen = (visitor.color(v) != DFSColorState::White);
                }
                else if constexpr (TracksColorState<Workspace>)
                {
                    seen = (workspace.color(v) != DFSColorState::White);
                }
                else
                {
                    static_assert(TracksColorState<Visitor>
                                  || TracksColorState<Workspace>,
                                  "dfs(graph, source, workspace, visitor): "
                                  "Expected either workspace or visitor to "
                                  "expose .color(v)");
                }

                if (!seen)
                {
                    visitor.tree_edge(u, v);

                    if constexpr (TracksColorState<Workspace>)
                    {
                        workspace.set_color(v, DFSColorState::Grey);
                    }

                    // Apply any filtering rules.
                    if constexpr (DiscoverControllable<Visitor>)
                    {
                        ControlAction action = visitor.discover_vertex(v);

                        if (action == ControlAction::Skip)
                        {
                            continue;
                        }
                        else if (action == ControlAction::Stop)
                        {
                            return;
                        }
                    }
                    else
                    {
                        visitor.discover_vertex(v);
                    }

                    stack.push(v);
                }
                else
                {
                    visitor.non_tree_edge(u, v);
                }
            }

            // Set node as finished.
            if constexpr (TracksColorState<Workspace>)
            {
                workspace.set_color(u, DFSColorState::Black);
            }

            visitor.finish_vertex(u);
        }
    }

    // Helper that creates a temporary workspace.
    template <AdjacencyGraph Graph,
              SearchVisitor Visitor>
    void dfs(const Graph & graph,
             VertexID source,
             Visitor&& visitor)
    {
        if constexpr (TracksColorState<Visitor>)
        {
            DFSWorkspace workspace;
            workspace.stack.reserve(graph.vertex_count());
            workspace.next();

            dfs(graph, source, workspace, std::forward<Visitor>(visitor));
        }
        else
        {
            DFSColorWorkspace workspace(graph.vertex_count());
            workspace.stack.reserve(graph.vertex_count());
            workspace.next();

            dfs(graph, source, workspace, std::forward<Visitor>(visitor));
        }
    }

    //
    // Visitor setups that can be reused.
    //

    // Since the timer can get to 2*|V| - 1, it could be
    // insufficient to set Numeric = VertexID::underlying_type
    // because the graph might be very deep.
    //
    // Instead, size_t should be able to easily hold the value
    // as it is infeasible to construct a graph large enough
    // that we overflow.
    template <typename Numeric = size_t>
    struct TimerDFSVisitor : NoOpSearchVisitor
    {
        using TimerStorage = std::vector<Numeric>;

        static constexpr Numeric NUMERIC_MAX
            = std::numeric_limits<Numeric>::max();

        TimerDFSVisitor(TimerStorage & d,
                        TimerStorage & f)
        :discovered(d),
        finished(f)
        {}

        void discover_vertex(VertexID u)
        {
            discovered[u.value] = timer++;
        }

        void finish_vertex(VertexID u)
        {
            finished[u.value] = timer++;
        }

        // For returning the color state information
        // that DFS needs during its traversal.
        DFSColorState color(VertexID u) const
        {
            // If discovered[u] == NUMERIC_MAX then we have
            // never visited this node during our traversal.
            if (discovered[u.value] == NUMERIC_MAX)
            {
                return DFSColorState::White;
            }
            // Otherwise, either finished[u] == NUMERIC_MAX and the node
            // is currently in the stack, or finished[u] != NUMERIC_MAX
            // and therefore we have finished with this node.
            else
            {
                return finished[u.value] == NUMERIC_MAX
                       ? DFSColorState::Grey : DFSColorState::Black;
            }
        }

        size_t time() const
        {
            return timer;
        }

    private:
        size_t timer{0};
        TimerStorage & discovered;
        TimerStorage & finished;
    };

    //
    // DFS utility functions.
    //

    // For users who want a pair of {discovered, finished}
    // arrays for DFS timestamps.
    template <AdjacencyGraph Graph,
              DFSCompatibleWorkspace Workspace>
    auto dfs_timestamps(const Graph & graph,
                        VertexID source,
                        Workspace & workspace)
    {
        using Visitor = TimerDFSVisitor<>;

        size_t V_count = graph.vertex_count();
        Visitor::TimerStorage d(V_count);
        Visitor::TimerStorage f(V_count);

        Visitor visitor(d, f);

        dfs(graph, source, workspace, visitor);

        return std::pair{d, f};
    }

    // Temporary workspace overload.
    template <AdjacencyGraph Graph>
    auto dfs_timestamps(const Graph & graph, VertexID source)
    {
        size_t V_count = graph.vertex_count();

        DFSWorkspace workspace;
        workspace.stack.reserve(V_count);
        workspace.next();

        return std::move(dfs_timestamps(graph, source, workspace));
    }
};
