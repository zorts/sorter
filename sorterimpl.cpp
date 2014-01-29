// -*- mode: c++ -*-

#include "sorterimpl.h"

namespace external_sort {

  SorterImpl::SorterImpl(unsigned int runSize, bool stable, Receiver* receiver)
    : _receiver(receiver)
    , _runSize(runSize)
    , _stable(stable)
    , _firstRun(true)
  {
    _currentRunState = getRunState();
  }

  SorterImpl::~SorterImpl()
  {
    // TBD
  }

  void SorterImpl::finish()
  {
    if (_firstRun)
    {
      // No merges are required
      _currentRunState->sort();
      const char* blockBase = _currentRunState->runBlock().data();
      for (auto keyPointer: _currentRunState->keyVector())
      {
        const PayloadItem* item = keyPointer.payload(blockBase);
        _receiver->receive(item->payloadData(), item->_payloadLength);
      }
    }
    else
    {
      // TBD: 
      addToRunQueue(_currentRunState);
      _currentRunState.reset();
      awaitMergeCompletion();
    }
  }
    
  RunStateSPtr SorterImpl::getRunState()
  {
    // TBD
    return RunStateSPtr(new RunState(_runSize, _stable));
  }
    
  void SorterImpl::addToRunQueue(RunStateSPtr)
  {
    _firstRun = false;
    // TBD
  }

  void SorterImpl::awaitMergeCompletion()
  {
    // TBD
  }

}