#ifndef TESTS_HPP
#define TESTS_HPP

#include "Brinjal.hpp"

struct Tests
{
    Tests(Brinjal* brinjal_ptr)
      : brinjal(brinjal_ptr)
    { }

    bool pilot();
    bool gfci();
    bool relay();

private:
    Brinjal* brinjal;
};

#endif // TESTS_HPP
