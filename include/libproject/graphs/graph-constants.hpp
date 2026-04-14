#pragma once
#include <cstdint>

#ifndef GRAPH_VERTEX_ID_TYPE
#define GRAPH_VERTEX_ID_TYPE uint32_t
#endif

#ifndef GRAPH_EDGE_ID_TYPE
#define GRAPH_EDGE_ID_TYPE uint32_t
#endif

// TODO: make this more strict (prevent VertexID <-> EdgeID conversions)
namespace libproject::graphs
{
    struct VertexID;
    struct EdgeID;

    struct VertexID
    {
        using underlying_type = GRAPH_VERTEX_ID_TYPE;

        static constexpr underlying_type max = std::numeric_limits<underlying_type>::max();
        static constexpr underlying_type empty = max;

        constexpr VertexID() noexcept
        :value(empty)
        {}

        constexpr VertexID(underlying_type v) noexcept
        :value(v)
        {}

        // explicitly remove EdgeID -> VertexID conversions.
        constexpr VertexID(const EdgeID &) = delete;
        constexpr VertexID & operator=(const EdgeID &) = delete;

        // Allow underlying_type -> VertexID conversions.
        constexpr VertexID& operator=(underlying_type rhs) noexcept
        {
            value = rhs;
            return *this;
        }

        constexpr auto operator<=>(const VertexID & other) const = default;

        underlying_type value;
    };

    static_assert(std::is_trivially_copyable_v<VertexID>);
    static_assert(sizeof(VertexID) == sizeof(GRAPH_VERTEX_ID_TYPE));

    struct EdgeID
    {
        using underlying_type = GRAPH_EDGE_ID_TYPE;

        static constexpr underlying_type max = std::numeric_limits<underlying_type>::max();
        static constexpr underlying_type empty = max;

        constexpr EdgeID() noexcept
        :value(empty)
        {}

        constexpr EdgeID(underlying_type v) noexcept
        :value(v)
        {}

        // explicitly remove VertexID -> EdgeID conversions.
        constexpr EdgeID(const VertexID &) = delete;
        constexpr EdgeID & operator=(const VertexID &) = delete;

        // Allow underlying_type -> EdgeID conversions.
        constexpr EdgeID& operator=(underlying_type rhs) noexcept
        {
            value = rhs;
            return *this;
        }

        constexpr auto operator<=>(const EdgeID & other) const = default;

        underlying_type value;
    };

    static_assert(std::is_trivially_copyable_v<EdgeID>);
    static_assert(sizeof(EdgeID) == sizeof(GRAPH_EDGE_ID_TYPE));

    // For global views over all edges
    struct EdgeView
    {
        VertexID from;
        VertexID to;
        EdgeID id;
    };

    static_assert(std::is_trivially_copyable_v<EdgeView>);

    // For views which already have the input vertex
    struct IncidentEdge
    {
        VertexID neighbor;
        EdgeID id;
    };

    static_assert(std::is_trivially_copyable_v<IncidentEdge>);

};
