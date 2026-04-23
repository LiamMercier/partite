#include <iostream>
#include <typeinfo>

#include <gtest/gtest.h>

#include <libproject/graphs/graph-constants.hpp>

using namespace libproject::graphs;

TEST(GraphConstants, VertexIDConstruction)
{
    {
        VertexID id;
        EXPECT_EQ(id.value, VertexID::empty);
    }

    {
        int v = 127;
        VertexID id(v);
        EXPECT_EQ(id.value, v);
    }
}

TEST(GraphConstants, EdgeIDConstruction)
{
    {
        EdgeID id;
        EXPECT_EQ(id.value, EdgeID::empty);
    }

    {
        int v = 127;
        EdgeID id(v);
        EXPECT_EQ(id.value, v);
    }
}

TEST(GraphConstants, VertexIDComparison)
{
    VertexID a(5);
    VertexID b(5);
    VertexID c(7);

    EXPECT_EQ(a, b);
    EXPECT_LT(b, c);
    EXPECT_GT(c, b);
}

TEST(GraphConstants, EdgeIDComparison)
{
    EdgeID a(5);
    EdgeID b(5);
    EdgeID c(7);
    EdgeID d(0);

    EXPECT_EQ(a, b);
    EXPECT_GT(c, b);
    EXPECT_LT(d, c);
}

TEST(GraphConstants, VertexIDEmpty)
{
    VertexID a(5);
    VertexID empty(VertexID::empty);

    EXPECT_GT(empty, a);
    EXPECT_EQ(empty.value, VertexID::empty);
}

TEST(GraphConstants, EdgeIDEmpty)
{
    EdgeID a(5);
    EdgeID empty(EdgeID::empty);

    EXPECT_GT(empty, a);
    EXPECT_EQ(empty.value, EdgeID::empty);
}

TEST(GraphConstants, VertexIDMax)
{
    EXPECT_EQ(VertexID::max, std::numeric_limits<GRAPH_VERTEX_ID_TYPE>::max());
}

TEST(GraphConstants, EdgeIDMax)
{
    EXPECT_EQ(EdgeID::max, std::numeric_limits<GRAPH_EDGE_ID_TYPE>::max());
}
