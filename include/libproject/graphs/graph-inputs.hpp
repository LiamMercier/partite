#pragma once

#include <ranges>

#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{
    // Check if EdgeData or VertexData is monostate.
    template <typename T>
    concept HasData = !std::is_same_v<T, std::monostate>;

    // For inputs to PropertyGraph
    template <typename EdgeData = std::monostate>
    struct EdgeInput
    {
        // Construct an EdgeInput from something like
        // {VertexID, VertexID}
        EdgeInput(VertexID f, VertexID t)
            requires (!HasData<EdgeData>)
        :from(f),
        to(t)
        {}

        // Construct an EdgeInput from something like
        // {VertexID, VertexID}
        EdgeInput(VertexID f, VertexID t, EdgeData d)
            requires (HasData<EdgeData>)
        :from(f),
        to(t),
        data(std::move(d))
        {}

        // Construct an EdgeInput from something like
        // {{VertexID, VertexID}, EdgeData}
        //
        // Example: {{0, 1}, "edge value"}
        EdgeInput(std::pair<VertexID, VertexID> verts, EdgeData e)
            requires (HasData<EdgeData>)
        :from(verts.first),
        to(verts.second),
        data(std::move(e))
        {
        }

        // Construct an EdgeInput from something like
        // {VertexID, VertexID, EdgeData}
        EdgeInput(std::tuple<VertexID, VertexID, EdgeData> edge)
            requires (HasData<EdgeData>)
        :from(std::get<0>(edge)),
        to(std::get<1>(edge)),
        data(std::get<2>(edge))
        {
        }

    public:
        VertexID from;
        VertexID to;

        // This might be monostate, if so, don't store memory.
        [[no_unique_address]] EdgeData data{};
    };

    template <>
    struct EdgeInput<std::monostate>
    {
        EdgeInput(VertexID f, VertexID t)
        :from(f),
        to(t)
        {}

        EdgeInput(std::pair<VertexID, VertexID> verts)
        :from(verts.first),
        to(verts.second)
        {
        }

    public:
        VertexID from;
        VertexID to;
    };

    template <typename T, typename EdgeData>
    concept EdgeInputObject = std::constructible_from<EdgeInput<EdgeData>, T>;

    template <typename T, typename EdgeData>
    concept EdgeInputRange = std::ranges::input_range<T> &&
                            EdgeInputObject<
                                std::ranges::range_value_t<T>, EdgeData>;

    template <typename T>
    concept EdgeViewObject = requires(T&& e)
    {
        { e.from } -> std::convertible_to<VertexID>;
        { e.to } -> std::convertible_to<VertexID>;
        { e.id } -> std::convertible_to<EdgeID>;
    };

    template <typename T>
    concept EdgeViewRange = std::ranges::input_range<T> &&
                            EdgeViewObject<std::ranges::range_value_t<T>>;

    // TODO: proper ID counting
    template <typename EdgeData, typename ER>
        requires EdgeInputRange<ER, EdgeData>
    auto make_edge_view_range(ER&& edge_range)
    {
        return std::forward<ER>(edge_range)
            | std::views::transform([](auto&& item){
                EdgeInput<EdgeData> e(std::forward<decltype(item)>(item));

                return EdgeView {
                    e.from, e.to, 0
                };
            });
    }
};
