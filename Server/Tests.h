#ifndef TESTS_H
#define TESTS_H

#include "Brinjal.h"

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

#endif // TESTS_H
