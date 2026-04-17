#include <gtest/gtest.h>

#include "test-utils.hpp"
#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>
#include <libproject/graphs/adjacency-list.hpp>

template <typename Graph, typename Edges>
std::pair<size_t, bool> assert_edges_exist(const Graph & graph,
                                           const Edges & edges)
{
    size_t index = 0;
    for (const auto & edge : edges)
    {
        bool exists = graph.has_edge(edge.from, edge.to);

        if (!exists)
        {
            return {index, false};
        }

        index++;
    }

    return {index, true};
}

using namespace libproject::graphs;

using TestGraphs = ::testing::Types
<
    // Simple graph (no VertexData or EdgeData)
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>>,
                std::monostate,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>>,
                std::monostate,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<true>>,
                std::monostate,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<false>>,
                std::monostate,
                std::monostate>,

    // EdgeData but no VertexData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, std::string>,
                std::string,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, std::string>,
                std::string,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<true>, std::string>,
                std::string,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<false>, std::string>,
                std::string,
                std::monostate>,

    // EdgeData and VertexData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, std::string, std::string>,
                std::string,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, std::string, std::string>,
                std::string,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyList<true>, std::string, std::string>,
                std::string,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyList<false>, std::string, std::string>,
                std::string,
                std::string>,

    // VertexData but no EdgeData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>,
                              std::monostate,
                              std::string>,
                std::monostate,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>,
                              std::monostate,
                              std::string>,
                std::monostate,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyList<true>,
                              std::monostate,
                              std::string>,
                std::monostate,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyList<false>,
                              std::monostate,
                              std::string>,
                std::monostate,
                std::string>
>;

// Test suite for each permutation of a graph of 4 vertices.
template <typename Config>
class PropertyGraphExhaustiveTest : public ::testing::Test
{
public:
    using Graph = typename Config::Graph;
    using EdgeData = typename Config::EdgeData;
    using VertexData = typename Config::VertexData;

    // We need to test the following number of graphs:
    //
    // Assumptions:
    // - We allow self loops
    // - Graphs may be directed or undirected
    // - Count isomorphic graphs, i.e do not skip E = {(1, 2)}
    //   just because we tested E = {(0, 1)}
    //
    // Directed case:
    //
    // We have |V| ways to choose a source, and |V| ways
    // to choose a target, so there are |V| * |V| possible
    // edges in the graph. Since each edge is either
    // included or not, there are 2^(|V| * |V|) graphs.
    //
    // For |V| = 4 we thus have 2^(4 * 4) = 2^16 graphs
    // which we need to test.
    //
    // Undirected case:
    //
    // For distinct vertices, we have (|V| choose 2) ways
    // to create an edge, and then we have |V| self loops. Thus,
    // there are ((|V| choose 2) + |V|) possible edges.
    //
    // Let C be the count of possible edges, then:
    //
    // |V| choose 2 = (|V| * (|V| - 1)) / 2
    //
    // C = (|V| * (|V| - 1)) / 2 + |V|
    // = (|V| * (|V| - 1)) / 2 + (2 * |V| / 2)
    // = (|V| * (|V| - 1) + 2|V|) / 2
    // = (|V|^2 - |V| + 2|V|) / 2
    // = (|V| * (|V| + 1)) / 2
    //
    // And since each edge is either included or not, we have
    // 2^((|V| * (|V| + 1)) / 2) possible graphs.
    //
    // For |V| = 4 we thus have 2^(4 * 5 / 2) = 2^10 graphs
    // which we need to test.
    static constexpr size_t v_count = 4;
    static constexpr size_t directed_graphs = 1 << (v_count * v_count);
    static constexpr size_t undirected_graphs = 1 << ((v_count
                                                       * (v_count + 1)) / 2);

    // Generator for each distinct labeled graph of 4 vertices.
    static std::vector<EdgeInput<EdgeData>> compute_edges(size_t i)
    {
        // For the directed case, we simply map the bit vector
        // of i to each edge. For example, we map (0, 0)
        // to be i[0], (1, 0) to be i[4 + 0], etc.
        //
        // We get the mapping for (u, v) as index = u * |V| + v
        //
        // Since we do not actually have a bit vector, we simply
        // bit shift and compute the mask to check the value.
        if constexpr (Graph::directed)
        {
            std::vector<EdgeInput<EdgeData>> edges;
            edges.reserve(v_count * v_count);

            for (size_t u = 0; u < v_count; u++)
            {
                for (size_t v = 0; v < v_count; v++)
                {
                    size_t index = u * v_count + v;
                    if (i & (size_t{1} << index))
                    {
                        /// Handle data vs no data cases.
                        if constexpr (HasData<EdgeData>)
                        {
                            // Encode the iteration and edge pair.
                            std::string label = std::to_string(i)
                                                + ":"
                                                + std::to_string(u)
                                                + "-"
                                                + std::to_string(v);

                            edges.push_back({u, v, std::move(label)});
                        }
                        else
                        {
                            edges.push_back({u, v});
                        }
                    }
                }
            }

            return edges;
        }
        // For the undirected case, we only need to check (u, v) and not
        // (v, u) when u < v, so we can iterate across each u, and then
        // across each v > u, and insert (u, v).
        //
        // To do this, we keep track of the bit.
        else
        {
            std::vector<EdgeInput<EdgeData>> edges;
            edges.reserve((v_count * (v_count + 1)) / 2);

            size_t index = 0;

            for (size_t u = 0; u < v_count; u++)
            {
                // Start at v = u, since (u, u - N)
                // is unnecessary to check.
                for (size_t v = u; v < v_count; v++)
                {
                    if (i & (size_t{1} << index))
                    {
                        /// Handle data vs no data cases.
                        if constexpr (HasData<EdgeData>)
                        {
                            // Encode the iteration and edge pair.
                            std::string label = std::to_string(i)
                                                + ":"
                                                + std::to_string(u)
                                                + "-"
                                                + std::to_string(v);

                            edges.push_back({u, v, std::move(label)});
                        }
                        else
                        {
                            edges.push_back({u, v});
                        }
                    }

                    index++;
                }
            }

            return edges;
        }
    }

    // Cache all graphs for significantly faster tests.
    static const std::vector<Graph> & all_graphs()
    {
        size_t num_graphs = Graph::directed ? directed_graphs
                                            : undirected_graphs;

        static std::vector<Graph> cache = [num_graphs]{
            std::vector<Graph> res;

            for (size_t i = 0; i < num_graphs; i++)
            {
                auto edges = compute_edges(i);
                res.emplace_back(v_count, edges);
            }

            return res;
        }();

        return cache;
    }

    // We simply generate a list of vertex labels "0, ..., |V| - 1"
    static std::vector<VertexData> compute_vertices()
    {
        std::vector<VertexData> vertices;
        vertices.reserve(v_count);

        for (size_t v = 0; v < v_count; v++)
        {
            vertices.push_back(std::to_string(v));
        }

        return vertices;
    }
};

TYPED_TEST_SUITE(PropertyGraphExhaustiveTest, TestGraphs);

TYPED_TEST(PropertyGraphExhaustiveTest, ValidateGraphs)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    size_t num_graphs = Graph::directed ? TestFixture::directed_graphs
                                        : TestFixture::undirected_graphs;

    size_t v_count = TestFixture::v_count;
    size_t i = 0;

    // Go over each possible graph, and do every test necessary.
    for (const auto & graph : TestFixture::all_graphs())
    {
        auto edges = TestFixture::compute_edges(i);
        i++;

        auto res = assert_edges_exist(graph, edges);
        ASSERT_TRUE(res.second) << "Graph "
                                << i
                                << " was missing edge ("
                                << edges[res.first].from.value
                                << " "
                                << edges[res.first].to.value
                                << ")";

        // TODO: different tests.

        continue;
    }
}
