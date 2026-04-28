#pragma once

#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/graph-concepts.hpp>

namespace libproject::graphs
{
    // Optional struct for reusing memory across DFS calls.
    struct DFSWorkspace
    {
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
    concept DFSWorkspaceCompatible = requires (W & w, VertexID u)
    {
        { w.stack };
        { w.stack.push(u) };
        { w.stack.pop() } -> std::convertible_to<VertexID>;
        { w.next() };
    };

    // For checking if a workspace or visitor has a .color(u)
    // method for checking the state of the node.
    template <typename T>
    concept TracksColorState = requires (T & t, VertexID u)
    {
        { t.color(u) } -> std::convertible_to<DFSColorState>;
    };

    template <typename V>
    concept DFSVisitor = requires(V & visitor, VertexID u, VertexID v)
    {
        { visitor.source_vertex(u) };
        { visitor.discover_vertex(u) };
        { visitor.expand_vertex(u) };
        { visitor.examine_edge(u, v) };
        { visitor.tree_edge(u, v) };
        { visitor.non_tree_edge(u, v) };
        { visitor.finish_vertex(u) };
    };

    template <typename Graph,
              DFSWorkspaceCompatible Workspace,
              DFSVisitor Visitor>
    void dfs(const Graph & graph,
             VertexID source,
             Workspace & workspace,
             Visitor&& visitor)
    {
        auto & stack = workspace.stack;
        stack.push(source);

        // While the stack has vertices to visit.
        while (stack.size() > 0)
        {
            // Pop the next vertex
            VertexID u = stack.pop();

            for (const VertexID & v : graph.neighbors(u))
            {
                bool seen = false;

                // If the visitor exposes .color
                // then we can use this instead of forcing
                // every user to allocate a third color array.
                if constexpr (TracksColorState<Visitor>)
                {
                    seen = (visitor.color(v) != DFSColorState::White);
                }
                // Otherwise, the workspace should have a color array.
                else if constexpr (TracksColorState<Workspace>)
                {
                    seen = (workspace.color(v) != DFSColorState::White);
                }
                else
                {
                    static_assert(!TracksColorState<Visitor>
                                  && TracksColorState<Workspace>,
                                  "DFS: Expected Visitor or Workspace to "
                                  "expose a .color(v) method.");
                }

                // If unseen
                if (!seen)
                {


                    if constexpr (TracksColorState<Workspace>)
                    {
                        workspace.set_color(v, DFSColorState::Grey);
                    }

                    stack.push(v);
                }
                else
                {
                    // TODO: visitor logic (later)
                }
            }

            // Set node as finished.
            if constexpr (TracksColorState<Workspace>)
            {
                workspace.set_color(u, DFSColorState::Black);
            }
        }
    }

    //
    // Visitor setups that can be reused.
    //

    struct NoOpDFSVisitor
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

    // Since the timer can get to 2*|V| - 1, it could be
    // insufficient to set Numeric = VertexID::underlying_type
    // because the graph might be very deep.
    //
    // Instead, size_t should be able to easily hold the value
    // as it is probably impossible to construct a graph large enough
    // that we overflow.
    template <typename Numeric = size_t>
    struct TimerDFSVisitor : NoOpDFSVisitor
    {
        using TimerStorage = std::vector<Numeric>;

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
        // that DFS needs during is traversal.
        DFSColorState color(VertexID u)
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

        size_t timer{0};
        TimerStorage & discovered;
        TimerStorage & finished;
        static constexpr Numeric NUMERIC_MAX
            = std::numeric_limits<Numeric>::max();
    };

};
