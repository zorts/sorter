// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#include "sorterimpl.h"

namespace external_sort {

  class NotYetSupportedException : public SorterException {
  public:
    NotYetSupportedException(const char* what)
      : _what(what) {}
    ~NotYetSupportedException() noexcept {}
    // default copy/assign OK
    virtual const char* what() const noexcept 
    {
      return _what.c_str();
    }
  private:
    std::string _what;
  };

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
    throw new NotYetSupportedException("addToRunQueue called");
  }

  void SorterImpl::awaitMergeCompletion()
  {
    // TBD
    throw new NotYetSupportedException("awaitMergeCompletion called");
  }

}
