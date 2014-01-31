// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#ifndef EXTERNAL_SORT_SORTER_H
#define EXTERNAL_SORT_SORTER_H

#include <exception>

namespace external_sort {

  class SorterException : public std::exception {};  

  class SorterNotCreatedException : public SorterException {
  public:
    // default ctor/dtor/copy/assign OK
    virtual const char* what() const noexcept 
    {
      return "The sorter was not created (by calling create()).";
    }
  };

  class SorterCreatedMoreThanOnceException : public SorterException {
  public:
    // default ctor/dtor/copy/assign OK
    virtual const char* what() const noexcept 
    {
      return "create() was called more than once.";
    }
  };

  class NoReceiverException : public SorterException {
  public:
    // default ctor/dtor/copy/assign OK
    virtual const char* what() const noexcept 
    {
      return "A receiver was not provided (by calling withReceiver()).";
    }
  };

  class RecordSizeException : public SorterException {
  public:
    RecordSizeException(unsigned int keyLength, 
                        unsigned int payloadLength,
                        unsigned int runBlockSize)
      : _keyLength(keyLength)
      , _payloadLength(payloadLength)
      , _runBlockSize(runBlockSize) {}

    virtual const char* what() const noexcept 
    {
      return "The record size exceeds the run block size";
    }

    unsigned int keyLength() const noexcept { return _keyLength; }
    unsigned int payloadLength() const noexcept { return _payloadLength; }
    unsigned int runBlockSize() const noexcept { return _runBlockSize; }
  private:
    // default dtor/copy/assign OK
    unsigned int _keyLength;
    unsigned int _payloadLength;
    unsigned int _runBlockSize;
  };

  class SorterImpl;

  class Receiver {
  public:
    virtual ~Receiver();
    virtual void receive(const void* payload, unsigned int payloadLength) = 0;
  protected:
    Receiver() {}
    // default copy/assign OK
  };

  class Sorter {
  public:
    Sorter();
    ~Sorter();

    // parameterization
    Sorter& withRunSize(unsigned int runSize); // Defaults to 64MB
    Sorter& withReceiver(Receiver*); 
    Sorter& stable(); // defaults to not stable
    Sorter& setStable(bool makeStable);
    void create();

    void sort(const void* key, unsigned int keyLength,
              const void* payload, unsigned int payloadLength);
    void finish();

  private:
    // prohibit copy/assign; do not implement
    Sorter(const Sorter&);
    Sorter& operator=(const Sorter&);

    SorterImpl* _impl;
    Receiver* _receiver;
    unsigned int _runSize;
    bool _stable;

    void checkForCreation();
  };
}

#endif // EXTERNAL_SORT_SORTER_H
