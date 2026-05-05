#pragma once

#include "test-utils.hpp"

#include <unordered_map>

template <typename Config>
class SearchGraphFixture : public ::testing::Test
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
                                    std::string label = std::to_string(from)
                                                        + "-"
                                                        + std::to_string(to);

                                    return label;
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
};
