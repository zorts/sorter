// -*- mode: c++ -*-

#include "sorter.h"

#include <cstring> // for memcmp/memcpy
#include <vector>
#include <memory>
#include <algorithm>

namespace external_sort {

  struct KeyItem {
    unsigned int _dataOffset;
    unsigned int _keyLength;

    inline
    const char* keyData() const 
    {
      return ((char*)this) + sizeof(KeyItem);
    }

    inline 
    unsigned int keyLength() const
    {
      return _keyLength;
    }

    inline 
    unsigned int dataOffset() const
    {
      return _dataOffset;
    }

    bool less(const KeyItem& rhs) const
    {
      unsigned int leftLength = _keyLength;
      unsigned int rightLength = rhs._keyLength;
      size_t compareLength = (size_t) std::min(leftLength, rightLength);
      int result = memcmp(keyData(), rhs.keyData(), compareLength);
      if (result == 0) 
      {
        return (leftLength < rightLength ? true : false);
      }
      return (result < 0 ? true : false);
    }
  };

  struct KeyPointer {
    KeyItem* _key;
    KeyPointer() : _key(nullptr) {}
    KeyPointer(KeyItem* key) : _key(key) {}
    // default dtor/copy/assign OK

    inline
    bool operator< (const KeyPointer& rhs) const
    {
      return _key->less(*rhs._key);
    }

    inline
    explicit operator bool() const 
    {
      return _key != nullptr;
    }
  };
  typedef std::vector<KeyPointer> KeyVector;

  class RunBlock {
  public:
    RunBlock(unsigned int size)
      : _data(new char[size])
      , _size(size)
      , _keyOffset(0)
      , _dataOffset(size)
    {}

    ~RunBlock()
    {
      delete [] _data;
      _data = nullptr;
    }

    static inline 
    unsigned int spaceNeededfor(unsigned int keyLength, unsigned int payloadLength)
    {
      return keyLength + payloadLength + sizeof(KeyItem) + sizeof(unsigned int);
    }

    inline
    const char* data() const {
      return _data;
    }

  private:

    inline
    unsigned int storeData(const void* payload, unsigned int payloadLength)
    {
      char* dataLoc = _data + (_dataOffset - payloadLength);
      memcpy(dataLoc, payload, (size_t) payloadLength);
      unsigned int* dataLength = (unsigned int*)(dataLoc - sizeof(unsigned int));
      *dataLength = payloadLength;
      _dataOffset -= (payloadLength + sizeof(unsigned int));
      return _dataOffset;
    }

    inline
    KeyPointer storeKey(const void* key, unsigned int keyLength,
                        unsigned int dataOffset)
    {
      KeyItem* keyItem = (KeyItem*) (_data + _keyOffset);
      keyItem->_dataOffset = dataOffset;
      keyItem->_keyLength = keyLength;
      char* keyLoc = _data + _keyOffset + sizeof(KeyItem);
      memcpy(keyLoc, key, (size_t) keyLength);
      _keyOffset += keyLength + sizeof(KeyItem);
      return KeyPointer(keyItem);
    }

  public:

    KeyPointer store(const void* key, unsigned int keyLength,
                     const void* payload, unsigned int payloadLength)
    {
      unsigned int available = _dataOffset - _keyOffset;
      if (spaceNeededfor(keyLength, payloadLength) <= available)
      {
        unsigned int dataOffset = storeData(payload, payloadLength);
        return storeKey(key, keyLength, dataOffset);
      }
      else
      {
        // It doesn't fit. Could it ever fit?
        // TBD: Assumes that all run blocks will be the same size. Consider 
        //      allowing expansion for huge records?
        if (spaceNeededfor(keyLength, payloadLength) > _size)
        {
          throw new SorterRecordSizeException(keyLength, payloadLength, _size);
        }
        return KeyPointer(nullptr);
      }
    }

    void clear()
    {
      _keyOffset = 0;
      _dataOffset = _size;
    }

  private:
    // prohibit copy/assign; do not implement
    RunBlock(const RunBlock&);
    RunBlock& operator=(const RunBlock&);
    char* _data;
    unsigned int _size;
    unsigned int _keyOffset;
    unsigned int _dataOffset;
  };

  class RunState {
  public:
    RunState(unsigned int runBlockSize, bool stable)
      : _runBlock(runBlockSize)
      , _stable(stable)
    {
      // TBD: set an initial capacity for the key vector
    }
    // default dtor OK

    inline
    bool store(const void* key, unsigned int keyLength,
               const void* payload, unsigned int payloadLength)
    {
      KeyPointer p = _runBlock.store(key, keyLength, payload, payloadLength);
      if (p)
      {
        _keyVector.push_back(p);
      }
      return (bool) p;
    }

    void sort()
    {
      if (_stable)
      {
        std::stable_sort(_keyVector.begin(), _keyVector.end());
      }
      else
      {
        std::sort(_keyVector.begin(), _keyVector.end());
      }
    }

    void clear()
    {
      _keyVector.resize(0);
      _runBlock.clear();
    }

    const KeyVector& keyVector() const
    {
      return _keyVector;
    }

    const RunBlock& runBlock() const
    {
      return _runBlock;
    }

  private:
    // prohibit copy/assign; do not implement
    RunState(const RunState&);
    RunState& operator=(const RunState&);

    KeyVector _keyVector;
    RunBlock _runBlock;
    bool _stable;
  };
  typedef std::shared_ptr<RunState> RunStateSPtr;

  class SorterImpl {
  public:
    SorterImpl(unsigned int runSize, bool stable, Receiver* receiver)
      : _receiver(receiver)
      , _runSize(runSize)
      , _stable(stable)
      , _firstRun(true)
    {
      _currentRunState = getRunState();
    }

    ~SorterImpl()
    {
      // TBD
    }

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

    void finish()
    {
      if (_firstRun)
      {
        // No merges are required
        _currentRunState->sort();
        const char* blockBase = _currentRunState->runBlock().data();
        for (auto keyItem: _currentRunState->keyVector())
        {
          const char* payloadBase = blockBase + keyItem._key->dataOffset();
          const char* payload = payloadBase + sizeof(unsigned int);
          unsigned int payloadLength = *((const unsigned int*) payloadBase);
          _receiver->receive(payload, payloadLength);
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

  private:
    // prohibit copy/assign; do not implement
    SorterImpl(const SorterImpl&);
    SorterImpl& operator=(const SorterImpl&);

    RunStateSPtr _currentRunState;
    Receiver* _receiver;
    unsigned int _runSize;
    bool _stable;
    bool _firstRun;
    
    RunStateSPtr getRunState()
    {
      // TBD
      return RunStateSPtr(new RunState(_runSize, _stable));
    }
    
    void addToRunQueue(RunStateSPtr)
    {
      _firstRun = false;
      // TBD
    }

    void awaitMergeCompletion()
    {
      // TBD
    }

  };

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
      throw new SorterNoReceiverException();
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
