#include <gtest/gtest.h>

#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>

TEST(GraphRepresentations, Construction)
{
    using namespace libproject::graphs;

    AdjacencyMatrix<true> A(5);
}
