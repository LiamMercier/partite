#pragma once

#include <iostream>
#include <cstdlib>

template <typename T>
inline void libpartite_assert(bool cond, const T & msg)
{
#ifndef NDEBUG
    if (!cond)
    {
        std::cerr << "Assert failed!\n" << msg << "\n";
        std::abort();
    }
#endif
}
