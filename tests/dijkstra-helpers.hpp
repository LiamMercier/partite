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

    static const Graph & perfect_binary_tree(size_t h)
    {
        static std::unordered_map<size_t, Graph> graphs;

        auto iter = graphs.find(h);

        if (iter == graphs.end())
        {
            auto graph = [h]{
                std::vector<EdgeInput<EdgeData>> edges;

                if (h == 0)
                {
                    return Graph(0, edges);
                }

                using id_type = VertexID::underlying_type;

                id_type n = (1ULL << (h+1)) - 1;

                for (id_type i = 0; i < n; i++)
                {
                    id_type left = 2*i + 1;
                    id_type right = 2*i + 2;

                    if (left < n)
                    {
                        if constexpr (HasData<EdgeData>)
                        {
                            edges.push_back({VertexID{i},
                                            VertexID{left},
                                            static_cast<int>(left)});
                        }
                        else
                        {
                            edges.push_back({VertexID{i}, VertexID{left}});
                        }
                    }

                    if (right < n)
                    {
                        if constexpr (HasData<EdgeData>)
                        {
                            edges.push_back({VertexID{i},
                                            VertexID{right},
                                            static_cast<int>(right)});
                        }
                        else
                        {
                            edges.push_back({VertexID{i}, VertexID{right}});
                        }
                    }
                }

                return Graph{n, edges};
            }();

            iter = graphs.emplace(h, std::move(graph)).first;
        }

        return iter->second;
    }
};
