// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#include "diskrun.h"
#include "gtest/gtest.h"

#include <vector>

namespace {

  typedef std::pair<unsigned int, std::string> KeyPayloadPair;
  typedef std::vector<KeyPayloadPair> KeyPayloadVector;

  TEST(DiskRun, SimpleOK1)
  {
    KeyPayloadVector source;
    source.push_back({3, "three"});
    source.push_back({2, "two"});
    source.push_back({1, "one"});
    source.push_back({101, "one hundred and one"});

    external_sort::DiskRunSPtr run = external_sort::DiskRun::getDiskRun(0,1,1);
    for (auto item : source)
    {
      run->write(&item.first, sizeof(KeyPayloadPair::first_type),
                item.second.data(), item.second.size());
    }

    run->resetForRead();

    for (auto item : source)
    {
      ASSERT_TRUE(run->next());
      external_sort::DiskRun::Item key = run->getKey();
      external_sort::DiskRun::Item payload = run->getPayload();
      ASSERT_EQ(key._length, sizeof(KeyPayloadPair::first_type));
      ASSERT_EQ(item.first, *((const unsigned int*)key._data));
      ASSERT_EQ(payload._length, item.second.size());
      std::string payloadValue((const char*)payload._data, payload._length);
      ASSERT_EQ(payloadValue, item.second);
    }
    ASSERT_TRUE(!run->next());
  }
}
