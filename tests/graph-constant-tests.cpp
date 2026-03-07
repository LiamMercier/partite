#include <iostream>
#include <typeinfo>

#include <gtest/gtest.h>

#include <libproject/graphs/graph-constants.hpp>

TEST(GraphConstants, IDTypes)
{
    std::cout << typeid(libproject::Graphs::vertex_id).name()
              << " "
              << typeid(libproject::Graphs::edge_id).name();
}
