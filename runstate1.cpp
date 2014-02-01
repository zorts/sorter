// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#include "runstate.h"
#include "keyconvert.h"
#include "gtest/gtest.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include <stdint.h>

namespace {

  typedef ::external_sort::RunState RunStateSPtr;
  static const unsigned int RUN_BLOCK_SIZE = 1 * 1024 * 1024; 

  template <typename KT, typename PT>
  class RunStateSortTest : public ::testing::Test,
                           public ::external_sort::Receiver
  {
  protected:
    typedef std::pair<KT, PT> KeyPayloadPair;
    typedef std::vector<KeyPayloadPair> KeyPayloadVector;
    typedef std::vector<PT> PayloadVector;

    KeyPayloadVector source;
    PayloadVector result;
    ::external_sort::RunState sorter;

    RunStateSortTest()
      : sorter(RUN_BLOCK_SIZE, true)
    {
     
    }

    virtual std::string prepareKey(const KT& key) = 0;
    virtual std::string preparePayload(const PT& payload) = 0;
    virtual PT reconstructPayload(const void* payload, unsigned int payloadLength) = 0;

    void push_back(const KT& key, const PT& payload)
    {
      source.push_back({key, payload});
    }

    void doTheSort()
    {
      for (auto kp: source)
      {
        std::string preppedKey = prepareKey(kp.first);
        std::string preppedPayload = preparePayload(kp.second);
        sorter.store(preppedKey.data(), preppedKey.size(),
                     preppedPayload.data(), preppedPayload.size());
      }
      sorter.sort(this);
    }

    void receive(const void* payload, unsigned int payloadLength)
    {
      result.push_back(reconstructPayload(payload, payloadLength));
    }

    struct less {
      bool operator() (const KeyPayloadPair& l, const KeyPayloadPair& r)
      {
        return l.first < r.first;
      }
    };

    void checkResult() {
      ASSERT_EQ(source.size(), result.size())
        << "Source and result vector lengths differ. source: "
        << source.size() << ", result: " << result.size();

      std::stable_sort(source.begin(), source.end(), less());

      for (unsigned int i = 0; i < source.size(); ++i)
      {
        EXPECT_EQ(source[i].second, result[i])
          << "difference at " << i << ". source: (" 
          << source[i].first << ", " << source[i].second 
          << "), result: " << result[i];
      }
    }

    void sortAndCheck()
    {
      doTheSort();
      checkResult();
    }
  };

  template <typename KT>
  class WhateverStringRunStateSortTest : public RunStateSortTest<KT, std::string> {
  protected:
    virtual std::string preparePayload(const std::string& payload)
    {
      return payload;
    }

    virtual std::string reconstructPayload(const void* payload, unsigned int payloadLength)
    {
      return std::string((const char*) payload, payloadLength);
    }
  };

  class Uint32StringRunStateSortTest: public WhateverStringRunStateSortTest<uint32_t> {
    virtual std::string prepareKey(const uint32_t& key)
    {
      char buffer[4];
      ::external_sort::uint32ToKey(key, buffer);
      return std::string(buffer, 4);
    }
  };

  TEST_F(Uint32StringRunStateSortTest, Basic)
  {
    using namespace std;
    push_back(3, "three 1");
    push_back(2, "two");
    push_back(1, "one");
    push_back(0, "zero");
    push_back(3, "three 2");
    sortAndCheck();
  }

  class Int32StringRunStateSortTest: public WhateverStringRunStateSortTest<int32_t> {
    virtual std::string prepareKey(const int32_t& key)
    {
      char buffer[4];
      ::external_sort::int32ToKey(key, buffer);
      return std::string(buffer, 4);
    }
  };

  TEST_F(Int32StringRunStateSortTest, Basic)
  {
    using namespace std;
    push_back(3, "three 1");
    push_back(2, "two");
    push_back(1, "one");
    push_back(0, "zero");
    push_back(3, "three 2");
    push_back(-1, "minus one");
    sortAndCheck();
  }

  class DoubleStringRunStateSortTest : public WhateverStringRunStateSortTest<double> {
  protected:
    virtual std::string prepareKey(const double& key)
    {
      char buffer[8];
      ::external_sort::doubleToKey(key, buffer);
      return std::string(buffer, 8);
    }
  };

  TEST_F(DoubleStringRunStateSortTest, Basic)
  {
    using namespace std;
    push_back(3.0, "three point oh");
    push_back(2, "two");
    push_back(1, "one");
    push_back(0, "zero");
    push_back(3, "three point oh 2");
    push_back(-1, "minus one");
    sortAndCheck();
  }

  class StringStringRunStateSortTest : public WhateverStringRunStateSortTest<std::string> {
  protected:
    virtual std::string prepareKey(const std::string& key)
    {
      return key;
    }
  };

  TEST_F(StringStringRunStateSortTest, Basic)
  {
    using namespace std;
    push_back("a", "letter a");
    push_back("aa", "letters aa");
    push_back("b", "letter b");
    push_back("z", "letter z");
    push_back("c", "letter c");
    sortAndCheck();
  }
}
