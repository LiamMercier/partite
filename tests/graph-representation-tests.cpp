#include <gtest/gtest.h>

#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>
#include <libproject/graphs/adjacency-list.hpp>

#include <unordered_map>
#include <unordered_set>

using namespace libproject::graphs;

TEST(GraphRepresentations, AdjacencyMatrixConstruction)
{
    {
        AdjacencyMatrix<true> A(5);

        for (int i = 0; i < A.vertex_count(); i++)
        {
            for (int j = 0; j < A.vertex_count(); j++)
            {
                // There should be no valid edges
                EXPECT_FALSE(A.has_edge(i, j));
                EXPECT_EQ(A.find_edge(i, j), EdgeID::empty);
            }

            // Test adding self edges
            A.add_edge(i, i, 0);
        }

        // Test removing edges
        for (int i = 0; i < A.vertex_count(); i++)
        {
            // There should be a valid edge
            EXPECT_TRUE(A.has_edge(i, i));
            EXPECT_EQ(A.find_edge(i, i), 0);

            for (int j = 0; j < A.vertex_count(); j++)
            {
                A.remove_edge(i, j);
            }

            EXPECT_FALSE(A.has_edge(i, i));
            EXPECT_EQ(A.find_edge(i, i), EdgeID::empty);
        }
    }

    {
        AdjacencyMatrix<false> A(5);

        for (int i = 0; i < A.vertex_count(); i++)
        {
            for (int j = 0; j < A.vertex_count(); j++)
            {
                // There should be no valid edges
                EXPECT_FALSE(A.has_edge(i, j));
                EXPECT_EQ(A.find_edge(i, j), EdgeID::empty);
            }

            // Test adding self edges
            A.add_edge(i, i, 0);
        }

        // Test removing edges
        for (int i = 0; i < A.vertex_count(); i++)
        {
            // There should be a valid edge
            EXPECT_TRUE(A.has_edge(i, i));
            EXPECT_EQ(A.find_edge(i, i), 0);

            for (int j = 0; j < A.vertex_count(); j++)
            {
                A.remove_edge(i, j);
            }

            EXPECT_FALSE(A.has_edge(i, i));
            EXPECT_EQ(A.find_edge(i, i), EdgeID::empty);
        }
    }

}

// Test that we can properly represent an example graph.
TEST(GraphRepresentations, AdjacencyMatrixExample)
{
    // We will use the undirected graph G = (V, E) where
    // V = {0, 1, 2, 3}
    // E = {(0, 1), (1, 2), (1, 3)}
    AdjacencyMatrix<false> A(4);
    A.add_edge(0, 1, 0);
    A.add_edge(1, 2, 1);
    A.add_edge(1, 3, 2);

    // Do some lookup operations
    EXPECT_TRUE(A.has_edge(0, 1));
    EXPECT_TRUE(A.has_edge(1, 2));
    EXPECT_TRUE(A.has_edge(1, 3));

    EXPECT_EQ(A.find_edge(0, 1), 0);
    EXPECT_EQ(A.find_edge(1, 2), 1);
    EXPECT_EQ(A.find_edge(1, 3), 2);

    // Remove an edge
    A.remove_edge(0, 1);

    EXPECT_FALSE(A.has_edge(0, 1));
    EXPECT_EQ(A.find_edge(0, 1), EdgeID::empty);

    // Try adding the edge back, everything should work the same.
    A.add_edge(0, 1, 0);
    EXPECT_TRUE(A.has_edge(0, 1));
    EXPECT_EQ(A.find_edge(0, 1), 0);
}

TEST(GraphRepresentations, AdjacencyMatrixPairOverloads)
{
    // We will use the undirected graph G = (V, E) where
    // V = {0, 1, 2, 3}
    // E = {(0, 1), (1, 2), (1, 3)}
    AdjacencyMatrix<false> A(4);
    A.add_edge({0, 1}, 0);
    A.add_edge({1, 2}, 1);
    A.add_edge({1, 3}, 2);

    // Do some lookup operations
    EXPECT_TRUE(A.has_edge({0, 1}));
    EXPECT_TRUE(A.has_edge({1, 2}));
    EXPECT_TRUE(A.has_edge({1, 3}));

    EXPECT_EQ(A.find_edge({0, 1}), 0);
    EXPECT_EQ(A.find_edge({1, 2}), 1);
    EXPECT_EQ(A.find_edge({1, 3}), 2);

    // Remove an edge
    A.remove_edge({0, 1});

    EXPECT_FALSE(A.has_edge({0, 1}));
    EXPECT_EQ(A.find_edge({0, 1}), EdgeID::empty);

    // Try adding the edge back, everything should work the same.
    A.add_edge({0, 1}, 0);
    EXPECT_TRUE(A.has_edge({0, 1}));
    EXPECT_EQ(A.find_edge({0, 1}), 0);
}

using TestRepresentations = ::testing::Types
<
    AdjacencyMatrix<true>,
    AdjacencyMatrix<false>,
    AdjacencyList<true>,
    AdjacencyList<false>
>;

// TODO: make another test suite with each permutation
//       for some |V| = N
template <typename Rep>
class RepresentationCore : public ::testing::Test
{
public:
    using Representation = Rep;

    static std::vector<std::pair<VertexID, VertexID>> get_edges()
    {
        return {
            {0, 1}, {0, 2}, {0, 3},
            {1, 4},
            {2, 1}, {2, 3}, {2, 4},
            {3, 4}
        };
    }

    size_t v_count = 5;
};

TYPED_TEST_SUITE(RepresentationCore, TestRepresentations);

TYPED_TEST(RepresentationCore, EdgeOperations)
{
    using Representation = typename TestFixture::Representation;
    auto edges = TestFixture::get_edges();

    Representation rep = [&]() -> Representation
    {
        return Representation(TestFixture::v_count);
    }();

    size_t i = 0;
    for (const auto & e : edges)
    {
        rep.add_edge(e.first, e.second, i);
        i++;
    }

    for (const auto & e : edges)
    {
        ASSERT_TRUE(rep.has_edge(e.first, e.second));
    }

    {
        auto edge_pair = edges[0];
        ASSERT_EQ(rep.find_edge(edge_pair.first, edge_pair.second), 0);
    }

    auto edge_pair = edges.back();
    edges.pop_back();
    rep.remove_edge(edge_pair.first, edge_pair.second);

    for (const auto & e : edges)
    {
        ASSERT_TRUE(rep.has_edge(e.first, e.second));
    }

    ASSERT_FALSE(rep.has_edge(edge_pair.first, edge_pair.second));
    ASSERT_EQ(rep.find_edge(edge_pair.first, edge_pair.second), EdgeID::empty);
}

TYPED_TEST(RepresentationCore, VertexOperations)
{
    using Representation = typename TestFixture::Representation;
    auto edges = TestFixture::get_edges();

    if constexpr (!VertexMutable<Representation>)
    {
        return;
    }
    else
    {
        Representation rep = [&]() -> Representation
        {
            return Representation(TestFixture::v_count);
        }();

        size_t i = 0;
        for (const auto & e : edges)
        {
            rep.add_edge(e.first, e.second, i);
            i++;
        }

        // Add some vertices
        rep.add_vertex((TestFixture::v_count));
        rep.add_vertex((TestFixture::v_count + 1));
        rep.add_vertex((TestFixture::v_count + 2));
        rep.add_vertex((TestFixture::v_count + 3));

        // Add some edges
        rep.add_edge(0, 8, i++);
        rep.add_edge(1, 5, i++);
        rep.add_edge(1, 7, i++);
        rep.add_edge(8, 7, i++);
        rep.add_edge(7, 6, i++);
        rep.add_edge(6, 5, i++);

        edges.push_back({0, 8});
        edges.push_back({1, 5});
        edges.push_back({1, 7});
        edges.push_back({8, 7});
        edges.push_back({7, 6});
        edges.push_back({6, 5});

        for (const auto & e : edges)
        {
            ASSERT_TRUE(rep.has_edge(e.first, e.second));
        }

        // Remove node 6
        rep.remove_vertex(6);

        // Add the vertex back, any (u, 6) or (6, u) should be gone.
        rep.add_vertex(6);

        std::vector<std::pair<VertexID, VertexID>> bad_edges = {
            {7, 6}, {6, 5}
        };

        edges.pop_back();
        edges.pop_back();

        for (const auto & e : edges)
        {
            ASSERT_TRUE(rep.has_edge(e.first, e.second));
        }

        for (const auto & e : bad_edges)
        {
            ASSERT_FALSE(rep.has_edge(e.first, e.second));
        }
    }
}

TYPED_TEST(RepresentationCore, Clear)
{
    using Representation = typename TestFixture::Representation;
    auto edges = TestFixture::get_edges();

    if constexpr (!Clearable<Representation>)
    {
        return;
    }
    else
    {
        Representation rep = [&]() -> Representation
        {
            return Representation(TestFixture::v_count);
        }();

        size_t i = 0;
        for (const auto & e : edges)
        {
            rep.add_edge(e.first, e.second, i);
            i++;
        }

        for (const auto & e : edges)
        {
            ASSERT_TRUE(rep.has_edge(e.first, e.second));
        }

        rep.clear();

        for (const auto & e : edges)
        {
            ASSERT_FALSE(rep.has_edge(e.first, e.second));
        }
    }
}

TYPED_TEST(RepresentationCore, Degree)
{
    using Representation = typename TestFixture::Representation;
    auto edges = TestFixture::get_edges();

    // If we cannot call degree and cannot call both
    // in_degree and out_degree
    if constexpr (!DegreeQueryable<Representation>
                  && !(OutDegreeQueryable<Representation>
                       && InDegreeQueryable<Representation>))
    {
        return;
    }
    else
    {
        Representation rep = [&]() -> Representation
        {
            return Representation(TestFixture::v_count);
        }();

        size_t i = 0;
        for (const auto & e : edges)
        {
            rep.add_edge(e.first, e.second, i);
            i++;
        }

        for (const auto & e : edges)
        {
            ASSERT_TRUE(rep.has_edge(e.first, e.second));
        }

        std::vector<size_t> in(TestFixture::v_count);
        std::vector<size_t> out(TestFixture::v_count);

        // Compute the degree of each vertex.
        for (const auto & e : edges)
        {
            in[e.second.value] += 1;
            out[e.first.value] += 1;
        }

        for (size_t i = 0; i < TestFixture::v_count; i++)
        {
            ASSERT_EQ(rep.degree(i), in[i] + out[i]);

            if constexpr (Representation::directed)
            {
                ASSERT_EQ(rep.in_degree(i), in[i]);
                ASSERT_EQ(rep.out_degree(i), out[i]);
            }
        }

        // Remove two edges.
        {
            auto & e = edges.back();
            edges.pop_back();
            in[e.second.value] -= 1;
            out[e.first.value] -= 1;
            rep.remove_edge(e.first, e.second);
        }

        {
            auto & e = edges.back();
            edges.pop_back();
            in[e.second.value] -= 1;
            out[e.first.value] -= 1;
            rep.remove_edge(e.first, e.second);
        }

        // Check degrees again.
        for (size_t i = 0; i < TestFixture::v_count; i++)
        {
            ASSERT_EQ(rep.degree(i), in[i] + out[i]);

            if constexpr (Representation::directed)
            {
                ASSERT_EQ(rep.in_degree(i), in[i]);
                ASSERT_EQ(rep.out_degree(i), out[i]);
            }
        }
    }
}

TYPED_TEST(RepresentationCore, SelfLoops)
{
    using Representation = typename TestFixture::Representation;
    auto edges = TestFixture::get_edges();

    // If we cannot call degree and cannot call both
    // in_degree and out_degree
    if constexpr (!SelfLoopQueryable<Representation>)
    {
        return;
    }
    else
    {
        Representation rep = [&]() -> Representation
        {
            return Representation(TestFixture::v_count);
        }();

        // Add some more edges
        edges.push_back({4, 4});
        edges.push_back({3, 3});

        size_t i = 0;
        for (const auto & e : edges)
        {
            rep.add_edge(e.first, e.second, i);
            i++;
        }

        // Try to introduce errors.
        if constexpr (VertexMutable<Representation>)
        {
            rep.add_vertex(TestFixture::v_count);
        }

        for (const auto & e : edges)
        {
            ASSERT_TRUE(rep.has_edge(e.first, e.second));
        }

        if constexpr (VertexMutable<Representation>)
        {
            rep.remove_vertex(TestFixture::v_count);
        }

        std::vector<size_t> in(TestFixture::v_count);
        std::vector<size_t> out(TestFixture::v_count);
        std::vector<uint8_t> self_loops(TestFixture::v_count);

        // Compute the degree of each vertex, and check
        // for self loops.
        for (const auto & e : edges)
        {
            in[e.second.value] += 1;
            out[e.first.value] += 1;

            if (e.first.value == e.second.value)
            {
                self_loops[e.first.value] = 1;
            }
        }

        // Ensure the self loops are as expected, and check
        // the degree computation.
        for (size_t i = 0; i < TestFixture::v_count; i++)
        {
            if constexpr (DegreeQueryable<Representation>)
            {
                ASSERT_EQ(rep.degree(i), in[i] + out[i]);
            }

            if constexpr (Representation::directed)
            {
                if constexpr (InDegreeQueryable<Representation>)
                {
                    ASSERT_EQ(rep.in_degree(i), in[i]);
                }
                if constexpr (OutDegreeQueryable<Representation>)
                {
                    ASSERT_EQ(rep.out_degree(i), out[i]);
                }
            }

            ASSERT_EQ(rep.has_self_loop(i), self_loops[i]);
        }

        // Remove an edge.
        {
            auto & e = edges.back();
            edges.pop_back();
            in[e.second.value] -= 1;
            out[e.first.value] -= 1;

            if (e.first.value == e.second.value)
            {
                self_loops[e.first.value] = 0;
            }

            rep.remove_edge(e.first, e.second);
        }

        // Check everything again.
        for (size_t i = 0; i < TestFixture::v_count; i++)
        {
            if constexpr (DegreeQueryable<Representation>)
            {
                ASSERT_EQ(rep.degree(i), in[i] + out[i]);
            }

            if constexpr (Representation::directed)
            {
                if constexpr (InDegreeQueryable<Representation>)
                {
                    ASSERT_EQ(rep.in_degree(i), in[i]);
                }
                if constexpr (OutDegreeQueryable<Representation>)
                {
                    ASSERT_EQ(rep.out_degree(i), out[i]);
                }
            }

            ASSERT_EQ(rep.has_self_loop(i), self_loops[i]);
        }

        ASSERT_TRUE(rep.has_self_loops());

        if constexpr (!VertexMutable<Representation>)
        {
            return;
        }
        else
        {
            // Remove some vertices.
            rep.remove_vertex(4);
            rep.remove_vertex(3);

            std::vector<
                std::pair<VertexID,
                        VertexID>> edges_copy;

            // Setup our test values.
            in.clear();
            out.clear();
            self_loops.clear();

            in.resize(TestFixture::v_count - 2);
            out.resize(TestFixture::v_count - 2);
            self_loops.resize(TestFixture::v_count - 2);

            for (const auto & e : edges)
            {
                if (e.first.value == 4 || e.second.value == 4)
                {
                    continue;
                }

                if (e.first.value == 3 || e.second.value == 3)
                {
                    continue;
                }

                edges_copy.push_back(e);
            }

            for (const auto & e : edges_copy)
            {
                in[e.second.value] += 1;
                out[e.first.value] += 1;

                if (e.first.value == e.second.value)
                {
                    self_loops[e.first.value] = 1;
                }
            }

            // Check everything again.
            for (size_t i = 0; i < TestFixture::v_count - 2; i++)
            {
                if constexpr (DegreeQueryable<Representation>)
                {
                    ASSERT_EQ(rep.degree(i), in[i] + out[i]);
                }

                if constexpr (Representation::directed)
                {
                    if constexpr (InDegreeQueryable<Representation>)
                    {
                        ASSERT_EQ(rep.in_degree(i), in[i]);
                    }
                    if constexpr (OutDegreeQueryable<Representation>)
                    {
                        ASSERT_EQ(rep.out_degree(i), out[i]);
                    }
                }

                ASSERT_EQ(rep.has_self_loop(i), self_loops[i]);
            }

            ASSERT_FALSE(rep.has_self_loops());
        }
    }
}

// TYPED_TEST(RepresentationCore, Neighbors)
// {
//     using Representation = typename TestFixture::Representation;
//     auto edges = TestFixture::get_edges();
//
//     if constexpr (!Clearable<Representation>)
//     {
//         return;
//     }
//     else
//     {
//         Representation rep = [&]() -> Representation
//         {
//             return Representation(TestFixture::v_count);
//         }();
//
//         size_t i = 0;
//         for (const auto & e : edges)
//         {
//             rep.add_edge(e.first, e.second, i);
//             i++;
//         }
//
//         for (const auto & e : edges)
//         {
//             ASSERT_TRUE(rep.has_edge(e.first, e.second));
//         }
//
//         std::vector<std::unordered_set<size_t>> incoming(TestFixture::v_count);
//         std::vector<std::unordered_set<size_t>> outgoing(TestFixture::v_count);
//         size_t id = 0;
//
//         for (const auto & e : edges)
//         {
//             incoming[e.second.value].insert(id);
//             outgoing[e.first.value].insert(id);
//             id++;
//         }
//
//         // Check that each neighbor is as expected.
//         for (size_t v = 0; v < TestFixture::v_count; v++)
//         {
//             if (!Representation::directed)
//             {
//                 for (const auto & u : rep.neighbors(v))
//                 {
//                     ASSERT_TRUE(outgoing[v].contains())
//                 }
//             }
//             else
//             {
//
//             }
//
//         }
//     }
// }
