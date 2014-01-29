#include "sorter.h"
#include "keyconvert.h"
#include "gtest/gtest.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <stdint.h>

namespace {

  template <typename KT, typename PT>
  class InMemoryTest : public ::testing::Test,
                        public ::external_sort::Receiver
  {
  protected:
    typedef std::pair<KT, PT> KeyPayloadPair;
    typedef std::vector<KeyPayloadPair> KeyPayloadVector;
    typedef std::vector<PT> PayloadVector;

    KeyPayloadVector source;
    PayloadVector result;
    ::external_sort::Sorter sorter;

    InMemoryTest()
    {
      sorter
        .withReceiver(this)
        .stable();
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
      sorter.create();
      for (auto kp: source)
      {
        std::string preppedKey = prepareKey(kp.first);
        std::string preppedPayload = preparePayload(kp.second);
        sorter.sort(preppedKey.data(), preppedKey.size(),
                    preppedPayload.data(), preppedPayload.size());
      }
      sorter.finish();
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

  class Uint32StringInMemoryTest : public InMemoryTest<uint32_t, std::string> {
  protected:
    virtual std::string prepareKey(const uint32_t& key)
    {
      char buffer[4];
      ::external_sort::uint32ToKey(key, buffer);
      return std::string(buffer, 4);
    }

    virtual std::string preparePayload(const std::string& payload)
    {
      return payload;
    }

    virtual std::string reconstructPayload(const void* payload, unsigned int payloadLength)
    {
      return std::string((const char*) payload, payloadLength);
    }
  };

  TEST_F(Uint32StringInMemoryTest, Basic)
  {
    using namespace std;
    push_back(3, "three 1");
    push_back(2, "two");
    push_back(1, "one");
    push_back(0, "zero");
    push_back(3, "three 2");
    sortAndCheck();
  }
}
