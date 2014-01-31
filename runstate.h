// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#ifndef EXTERNAL_SORT_RUNSTATE_H
#define EXTERNAL_SORT_RUNSTATE_H

#include "sorter.h" // for exceptions

#include <cstring> // for memcmp/memcpy
#include <vector>
#include <memory>
#include <algorithm>

/*
  This header provides all of the functionality related to initially
  loading data into the sort and creating a sorted run. Since so much 
  of this needs to be inlined for performance, there is no corresponding
  .cpp file.
*/

namespace external_sort {

  struct PayloadItem {
    unsigned int _payloadLength;
    // char _payloadData[] follows this member, hence the following function

    inline
    char* payloadData()
    {
      return ((char*)this) + sizeof(PayloadItem);
    }

    inline
    const char* payloadData() const
    {
      return ((const char*)this) + sizeof(PayloadItem);
    }

    static inline 
    unsigned int itemSize(unsigned int payloadLength) {
      return sizeof(PayloadItem) + payloadLength;
    }

    inline 
    void store(const void* payload, unsigned int payloadLength)
    {
      _payloadLength = payloadLength;
      memcpy(payloadData(), payload, (size_t) payloadLength);
    }

  };

  struct KeyItem {
    // default ctor/dtor/copy/assign OK

    unsigned int _dataOffset;
    unsigned int _keyLength;
    // char _keyData[] follows these members, hence the following function

    inline
    char* keyData()
    {
      return ((char*)this) + sizeof(KeyItem);
    }

    inline
    const char* keyData() const
    {
      return ((char*)this) + sizeof(KeyItem);
    }

    static inline 
    unsigned int itemSize(unsigned int keyLength) {
      return sizeof(KeyItem) + keyLength;
    }

    inline
    void store(const void* key, unsigned int keyLength,
               unsigned int dataOffset)
    {
      _dataOffset = dataOffset;
      _keyLength = keyLength;
      memcpy(keyData(), key, (size_t) keyLength);
    }

    inline
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

    KeyPointer() {} // needed for vector initialization
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

    inline 
    const PayloadItem* payload(const char* blockBase) const
    {
      return (const PayloadItem*) (blockBase + _key->_dataOffset);
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
    unsigned int spaceNeededFor(unsigned int keyLength, unsigned int payloadLength)
    {
      return KeyItem::itemSize(keyLength) + PayloadItem::itemSize(payloadLength);
    }

    inline
    const char* data() const {
      return _data;
    }

  private:

    inline
    unsigned int storeData(const void* payload, unsigned int payloadLength)
    {
      unsigned int spaceNeeded = PayloadItem::itemSize(payloadLength);
      PayloadItem* item = (PayloadItem*) (_data + (_dataOffset - spaceNeeded));
      item->store(payload, payloadLength);
      _dataOffset -= spaceNeeded;
      return _dataOffset;
    }

    inline
    KeyPointer storeKey(const void* key, unsigned int keyLength,
                        unsigned int dataOffset)
    {
      KeyItem* keyItem = (KeyItem*) (_data + _keyOffset);
      keyItem->store(key, keyLength, dataOffset);
      _keyOffset += KeyItem::itemSize(keyLength);
      return KeyPointer(keyItem);
    }

  public:

    KeyPointer store(const void* key, unsigned int keyLength,
                     const void* payload, unsigned int payloadLength)
    {
      unsigned int available = _dataOffset - _keyOffset;
      if (spaceNeededFor(keyLength, payloadLength) <= available)
      {
        unsigned int dataOffset = storeData(payload, payloadLength);
        return storeKey(key, keyLength, dataOffset);
      }
      else
      {
        // It doesn't fit. Could it ever fit?
        // TBD: Assumes that all run blocks will be the same size. Consider 
        //      allowing expansion for huge records?
        if (spaceNeededFor(keyLength, payloadLength) > _size)
        {
          throw new RecordSizeException(keyLength, payloadLength, _size);
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
      // This initial capacity is based on the overhead for a key/payload pair 
      // key/payload pair size of 20 bytes (entirely arbitrary...).
      // It might be reasonable to compute an average for the first run and use 
      // that for subsequent runs.
      unsigned int bytesPerRecord = RunBlock::spaceNeededFor(8, 12);
      unsigned int keyPointerVectorLength = runBlockSize/bytesPerRecord;
      _keyVector.reserve(keyPointerVectorLength);
      clear();
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
        ++ _records;
        _keySize += keyLength;
        _payloadSize += payloadLength;
        unsigned int recordSize = keyLength + payloadLength;
        if (recordSize > _maxRecordSize)
        {
          _maxRecordSize = recordSize;
        }
      }
      return (bool) p;
    }

    // I want the sort mechanism to be improvable (using, for instance,
    // microruns that fit into cache followed by a merge) without changing
    // the upper layers. So - sort with direct output and sort with diskrun
    // creation are all in this layer.

    void sort(Receiver* receiver)
    {
      if (_stable)
      {
        std::stable_sort(_keyVector.begin(), _keyVector.end());
      }
      else
      {
        std::sort(_keyVector.begin(), _keyVector.end());
      }

      const char* blockBase = _runBlock.data();
      for (auto keyPointer: _keyVector)
      {
        const PayloadItem* item = keyPointer.payload(blockBase);
        receiver->receive(item->payloadData(), item->_payloadLength);
      }
    }

    void clear()
    {
      _keyVector.resize(0);
      _runBlock.clear();
      _records = 0;
      _keySize = 0;
      _payloadSize = 0;
      _maxRecordSize = 0;
    }

  private:
    // prohibit copy/assign; do not implement
    RunState(const RunState&);
    RunState& operator=(const RunState&);

    KeyVector _keyVector;
    RunBlock _runBlock;
    bool _stable;

    // per-run statistics
    unsigned int _records;
    unsigned int _keySize;
    unsigned int _payloadSize;
    unsigned int _maxRecordSize;
  };
  typedef std::shared_ptr<RunState> RunStateSPtr;

} // namespace external_sort

#endif //  EXTERNAL_SORT_RUNSTATE_H
