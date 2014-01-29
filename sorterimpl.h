// -*- mode: c++ -*-
#ifndef EXTERNAL_SORT_SORTERIMPL_H
#define EXTERNAL_SORT_SORTERIMPL_H

#include "sorter.h"
#include "runstate.h"

namespace external_sort {

  class SorterImpl {
  public:
    SorterImpl(unsigned int runSize, bool stable, Receiver* receiver);
    ~SorterImpl();

    inline
    void sort(const void* key, unsigned int keyLength,
              const void* payload, unsigned int payloadLength)
    {
      if (!_currentRunState)
      {
        _currentRunState = getRunState();
      }

      if (!_currentRunState->store(key, keyLength, payload, payloadLength))
      {
        // didn't fit
        addToRunQueue(_currentRunState);
        _currentRunState = getRunState();
      }
      
    }

    void finish();

  private:
    // prohibit copy/assign; do not implement
    SorterImpl(const SorterImpl&);
    SorterImpl& operator=(const SorterImpl&);

    RunStateSPtr _currentRunState;
    Receiver* _receiver;
    unsigned int _runSize;
    bool _stable;
    bool _firstRun;
    
    RunStateSPtr getRunState();
    void addToRunQueue(RunStateSPtr);
    void awaitMergeCompletion();
  };
}

#endif // EXTERNAL_SORT_SORTERIMPL_H
