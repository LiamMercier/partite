#pragma once
#include <vector>

#include <libproject/internal/libproject-assert.hpp>
#include <libproject/graphs/graph-constants.hpp>

namespace libproject::graphs
{


template <bool Directed>
class MultigraphAdjacencyList
{
public:
    static constexpr bool directed = Directed;
    static constexpr bool multigraph = true;

private:
    using ListStorage = std::vector<std::vector<std::pair<VertexID, EdgeID>>>;

private:
    std::conditional_t<
        directed,
        ListStorage,
        std::monostate> out_edges_;

    std::conditional_t<
        directed,
        ListStorage,
        std::monostate> in_edges_;

    std::conditional_t<
        !directed,
        ListStorage,
        std::monostate> adj_;

    // Per vertex state for self loops.
    std::conditional_t<!directed,
                       std::vector<uint8_t>,
                       std::monostate> self_loop_;

    size_t num_self_loops_{0};

};

};
