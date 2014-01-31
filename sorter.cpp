// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#include "sorterimpl.h"

namespace external_sort {

  static const unsigned int DEFAULT_RUN_BLOCK_SIZE = 64 * 1024 * 1024; 

  Sorter::Sorter()
    : _impl(nullptr)
    , _runSize(DEFAULT_RUN_BLOCK_SIZE)
    , _stable(false)
  {}

  Sorter::~Sorter()
  {
    delete _impl; _impl = nullptr;
  }

  Sorter& Sorter::withRunSize(unsigned int runSize)
  {
    // TBD: some sanity checking
    _runSize = runSize;
    return *this;
  }

  Sorter& Sorter::withReceiver(Receiver* receiver)
  {
    _receiver = receiver;
    return *this;
  }

  Sorter& Sorter::stable() {
    return setStable(true);
  }

  Sorter& Sorter::setStable(bool makeStable)
  {
    _stable = makeStable;
    return *this;
  }

  void Sorter::create() 
  {
    if (_impl)
    {
      throw new SorterCreatedMoreThanOnceException();
    }
    if (!_receiver)
    {
      throw new NoReceiverException();
    }
    _impl = new SorterImpl(_runSize, _stable, _receiver);
  }

  inline 
  void Sorter::checkForCreation()
  {
    if (!_impl)
    {
      throw new SorterNotCreatedException();
    }
  }

  void Sorter::sort(const void* key, unsigned int keyLength,
                    const void* payload, unsigned int payloadLength)
  {
    checkForCreation();
    _impl->sort(key, keyLength, payload, payloadLength);
  }

  void Sorter::finish()
  {
    checkForCreation();
    _impl->finish();
  }

  // This has to be here (well, somewhere other than the header) to get the vtable created.
  Receiver::~Receiver() {}

}
