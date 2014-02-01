// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#include "runstate.h"
#include "keyconvert.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <memory>

namespace {

  typedef std::chrono::high_resolution_clock clock;
  typedef std::chrono::high_resolution_clock::time_point instant;
  typedef std::chrono::nanoseconds interval;
  typedef std::shared_ptr<external_sort::RunState> RunStateSPtr;

  class RunStateTimingTest : public ::external_sort::Receiver
  {
  protected:
    RunStateSPtr _sorter;
    RunStateTimingTest() {}

    void receive(const void* payload, unsigned int payloadLength)
    {
      ;
    }

    virtual std::string prepareKey(unsigned int key) const = 0;
    virtual unsigned int keySize() const = 0;

    void loadData(unsigned int count)
    {
      // the payload doesn't matter
      char payload{'P'};

      while (count)
      {
        std::string preppedKey = prepareKey(count--);
        _sorter->store(preppedKey.data(), preppedKey.size(), &payload, sizeof(payload));
      }
    }

    interval timeToSort()
    {
      instant start = clock::now();
      _sorter->sort(this);
      instant stop = clock::now();
      return stop - start;
    }

  public:
    double nanosecondsPerRecord(unsigned int count, unsigned int iterations, bool stable)
    {
      unsigned int bytesPerRecord = external_sort::RunBlock::spaceNeededFor(keySize(), 1);
      // Make sure the block size never exceeds (magic number) 256MB
      static const unsigned int limit = 256*1024*1024;
      unsigned int maxRecords = limit/bytesPerRecord;
      if (count > maxRecords)
      {
        return -1;
      }
      _sorter.reset(new external_sort::RunState(limit, stable));
      double total = 0;
      // Adjust the iterations for small record counts, since there is so much variability
      if (count < 100) 
      {
        iterations *= 10;
      }
      for (unsigned int i = 0; i < iterations; ++i)
      {
        loadData(count);
        interval t = timeToSort();
        total += (double) t.count();
        _sorter->clear();
      }
      return (total/((double)iterations))/((double) count);
    }
  };

  class Uint32TimingTest: public RunStateTimingTest {
  protected:
    virtual std::string prepareKey(unsigned int key) const
    {
      char buffer[4];
      ::external_sort::uint32ToKey(key, buffer);
      return std::string(buffer, 4);
    }

    virtual unsigned int keySize() const
    {
      return sizeof(unsigned int);
    }
  };

}

int main(int argc, const char* const argv[])
{
  using namespace std;
  Uint32TimingTest tester;
  static const unsigned int limit = 1000000;
  static const unsigned int iterations = 10;
  cout << "  records | time (stable)" << endl;
  for (unsigned int i = 1; i <= limit; i *= 10)
  {
    double rate = tester.nanosecondsPerRecord(i, iterations, true);
    if (rate < 0) break;
    cout << setw(9) << i << " | " << rate << " nanoseconds/record" << endl;
  }

  cout << endl << "  records | time (not stable)" << endl;
  for (unsigned int i = 1; i <= limit; i *= 10)
  {
    double rate = tester.nanosecondsPerRecord(i, iterations, false);
    if (rate < 0) break;
    cout << setw(9) << i << " | " << rate << " nanoseconds/record" << endl;
  }
  return 0;
}
