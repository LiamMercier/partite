#pragma once

#include "test-utils.hpp"

template <typename Config>
class SearchGraphFixture : public ::testing::Test
{
public:
    using Graph = typename Config::Graph;
    using EdgeData = typename Config::EdgeData;
    using VertexData = typename Config::VertexData;

    static const Graph & perfect_binary_tree(size_t h)
    {
        static Graph tree = [h]{
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
                        std::string label = std::to_string(i)
                                            + "-"
                                            + std::to_string(left);

                        edges.push_back({VertexID{i},
                                         VertexID{left},
                                         std::move(label)});
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
                        std::string label = std::to_string(i)
                                            + "-"
                                            + std::to_string(right);

                        edges.push_back({VertexID{i},
                                         VertexID{right},
                                         std::move(label)});
                    }
                    else
                    {
                        edges.push_back({VertexID{i}, VertexID{right}});
                    }
                }
            }

            return Graph{n, edges};
        }();

        return tree;
    }
};
