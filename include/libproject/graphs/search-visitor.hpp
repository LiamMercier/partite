#pragma once

namespace libproject::graphs
{
    enum class ControlAction
    {
        Continue,
        Skip,
        Stop
    };

    template <typename V>
    concept SearchVisitor = requires(V & visitor,
                                     VertexID u,
                                     VertexID v,
                                     EdgeID e)
    {
        { visitor.source_vertex(u) };
        { visitor.discover_vertex(u) };
        { visitor.expand_vertex(u) };
        { visitor.examine_edge(u, v, e) };
        { visitor.tree_edge(u, v, e) };
        { visitor.non_tree_edge(u, v, e) };
        { visitor.finish_vertex(u) };
    };

    template <typename V>
    concept DiscoverControllable= requires (V & visitor,
                                               VertexID u)
    {
        { visitor.discover_vertex(u) } -> std::same_as<ControlAction>;
    };

    template <typename V>
    concept ExamineControllable = requires (V & visitor,
                                            VertexID u,
                                            VertexID v,
                                            EdgeID e)
    {
        { visitor.examine_edge(u, v, e) } -> std::same_as<ControlAction>;
    };

    struct NoOpSearchVisitor
    {
        void source_vertex(VertexID u)
        {}

        void discover_vertex(VertexID u)
        {}

        void expand_vertex(VertexID u)
        {}

        void examine_edge(VertexID u, VertexID v, EdgeID e)
        {}

        void tree_edge(VertexID u, VertexID v, EdgeID e)
        {}

        void non_tree_edge(VertexID u, VertexID v, EdgeID e)
        {}

        void finish_vertex(VertexID u)
        {}
    };

    template <typename SearchPredicate>
    struct VertexSearchVisitor : NoOpSearchVisitor
    {
        explicit VertexSearchVisitor(SearchPredicate p)
        :found(std::move(p))
        {}

        ControlAction discover_vertex(VertexID v)
            requires std::predicate<SearchPredicate, VertexID>
        {
            if (found(v))
            {
                target_found = v;
                return ControlAction::Stop;
            }

            return ControlAction::Continue;
        }

        void discover_vertex(VertexID v)
            requires (!std::predicate<SearchPredicate, VertexID>)
        {
        }

        VertexID target_found{VertexID::empty};
        SearchPredicate found;
    };
};
