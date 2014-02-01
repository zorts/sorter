// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#include "sortassert.h"
#include "gtest/gtest.h"

namespace {
  using namespace external_sort;

  TEST(Assertions, Pass)
  {
    ASSERT_NO_THROW({SORT_ASSERT(1 == 1);});
  }

  TEST(Assertions, Fail)
  {
    ASSERT_ANY_THROW({SORT_ASSERT(0 == 1);});
  }
}
