#pragma once

#include "test-utils.hpp"

template <typename Config>
class SearchGraphFixture : public ::testing::Test
{
public:
    using Graph = typename Config::Graph;
    using EdgeData = typename Config::EdgeData;
    using VertexData = typename Config::VertexData;

};
