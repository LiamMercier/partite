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

            size_t n = (1ULL << (h+1)) - 1;

            for (size_t i = 0; i < n; i++)
            {
                size_t left = 2*i + 1;
                size_t right = 2*i + 2;

                if (left < n)
                {
                    if constexpr (HasData<EdgeData>)
                    {
                        std::string label = std::to_string(i)
                                            + "-"
                                            + std::to_string(left);

                        edges.push_back({i, left, std::move(label)});
                    }
                    else
                    {
                        edges.push_back({i, left});
                    }
                }

                if (right < n)
                {
                    if constexpr (HasData<EdgeData>)
                    {
                        std::string label = std::to_string(i)
                                            + "-"
                                            + std::to_string(right);

                        edges.push_back({i, right, std::move(label)});
                    }
                    else
                    {
                        edges.push_back({i, right});
                    }
                }
            }

            return Graph{n, edges};
        }();

        return tree;
    }
};
