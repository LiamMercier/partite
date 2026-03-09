#include <iostream>
#include <typeinfo>

#include <gtest/gtest.h>

#include <libproject/graphs/graph-constants.hpp>

TEST(GraphConstants, IDTypes)
{
    std::cout << "\n"
              << typeid(decltype(libproject::graphs::VertexID::value)).name()
              << "\n"
              << typeid(decltype(libproject::graphs::EdgeID::value)).name()
              << "\n";

    libproject::graphs::VertexID v_id = 5;
}
