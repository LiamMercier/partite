#pragma once

#include "test-utils.hpp"

#include <unordered_map>

template <typename Config>
class DijkstraFixture : public ::testing::Test
{
public:
    using Graph = typename Config::Graph;
    using EdgeData = typename Config::EdgeData;
    using VertexData = typename Config::VertexData;

    using id_type = VertexID::underlying_type;

    static const Graph & perfect_binary_tree(size_t h)
    {
        static std::unordered_map<size_t, Graph> graphs;

        auto iter = graphs.find(h);

        if (iter == graphs.end())
        {
            auto graph = make_perfect_binary_tree<Graph>(h,
                            [](id_type from, id_type to) -> EdgeData
                            {
                                if constexpr (HasData<EdgeData>)
                                {
                                    return to;
                                }
                                else
                                {
                                    return EdgeData{};
                                }
                            }
                         );

            iter = graphs.emplace(h, std::move(graph)).first;
        }

        return iter->second;
    }

    static const Graph & grid_graph(size_t length)
    {
        static std::unordered_map<size_t, Graph> graphs;

        auto iter = graphs.find(length);

        if (iter == graphs.end())
        {
            auto graph = make_grid_graph<Graph>(length,
                            [](id_type from, id_type to) -> EdgeData
                            {
                                if constexpr (HasData<EdgeData>)
                                {
                                    return to;
                                }
                                else
                                {
                                    return EdgeData{};
                                }
                            }
                         );

            iter = graphs.emplace(length, std::move(graph)).first;
        }

        return iter->second;
    }

};
