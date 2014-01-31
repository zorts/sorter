// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#ifndef EXTERNAL_SORT_SORTASSERT_H
#define EXTERNAL_SORT_SORTASSERT_H

#include <sstream>
#include <stdexcept>

namespace external_sort {

  class SorterAssertion : public std::runtime_error
  {
  public:
    SorterAssertion(const char *assertion, const char *file, unsigned int line)
      : std::runtime_error(makeMessage(assertion, file, line)) {}
  private:
    static
    std::string makeMessage(const char *assertion, const char *file, unsigned int line)
    {
      using namespace std;
      ostringstream m;
      m << file << ":" << line << ": ASSERTION FAILED: " << assertion;
      return m.str();
    }
  };  

#define SORT_ASSERT(expr)                                       \
  do {                                                          \
    if (!(expr))                                                \
    {                                                           \
      throw new SorterAssertion(#expr, __FILE__, __LINE__);     \
    }                                                           \
  } while(false)

// Assume that we are debugging if not compiling optimized.a
#ifndef __OPTIMIZE__
#define SORT_ASSERT_DEBUGONLY(expr) \
        SORT_ASSERT(expr)
#else
#define SORT_ASSERT_DEBUGONLY(expr) 
#endif

} // namespace external_sort

#endif // EXTERNAL_SORT_SORTASSERT_H
