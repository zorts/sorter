// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#include "merger.h"
#include "diskrun.h"
#include "sorter.h"
#include "sortassert.h"

#include <cstring>
#include <algorithm>

namespace external_sort {

  struct MergeItem {
    DiskRun::Item _key;
    unsigned int _runIndex;

    MergeItem() {}
    MergeItem(const DiskRun::Item& key, unsigned int runIndex)
      : _key(key)
      , _runIndex(runIndex) {}
    // default dtor/copy/assign OK

    inline
    bool less(const MergeItem& rhs) const
    {
      unsigned int leftLength = _key._length;
      unsigned int rightLength = rhs._key._length;
      size_t compareLength = (size_t) std::min(leftLength, rightLength);
      int result = memcmp(_key._data, rhs._key._data, compareLength);
      if (result == 0) 
      {
        return (leftLength < rightLength ? true : false);
      }
      return (result < 0 ? true : false);
    }
  };

  struct RunOrder {
    bool operator() (const MergeItem& left, const MergeItem& right) const
    {
      // The heap needs to be ordered smallest to largest, so invert the
      // usual sense of order.
      return right.less(left);
    }
  };

  class MergeWriter {
  public:
    // default ctor/copy/assign OK
    virtual ~MergeWriter() {}
    virtual void writeFrom(const DiskRun* run) = 0;
  };

  class MergerImpl {
  public:
    MergerImpl() {}
    // default dtor OK
    void addSource(DiskRunSPtr source)
    {
      SORT_ASSERT(source);
      if (source->isWritable())
      {
        source->resetForRead();
      }
      if (source->next())
      {
        _sources.push_back(source);
        _mergeItems.emplace_back(source->getKey(), _mergeItems.size()-1);
      }
    }

    void merge(MergeWriter& target)
    {
      const unsigned int first = 0;
      unsigned int last = _mergeItems.size()-1;
      std::make_heap(_mergeItems.begin(), _mergeItems.end(), RunOrder());
      while (!_mergeItems.empty())
      {
        DiskRun* lowestRun = _sources[_mergeItems[first]._runIndex].get();
        target.writeFrom(lowestRun);
        std::pop_heap(_mergeItems.begin(), _mergeItems.end(), RunOrder()); // move front to back
        if (lowestRun->next())
        {
          // get the new key and float the item to its new place
          _mergeItems[last]._key = lowestRun->getKey();
          std::push_heap(_mergeItems.begin(), _mergeItems.end(), RunOrder());
        }
        else
        {
          // done with this run
          _sources[_mergeItems[last]._runIndex].reset();
          _mergeItems.resize(last--);
        }
      }
    }

  private:
    // Prohibit copy/assign; do not implement
    MergerImpl(const Merger&);
    MergerImpl& operator=(const Merger&);

    std::vector<DiskRunSPtr> _sources;
    std::vector<MergeItem> _mergeItems;
  };

  Merger::Merger()
    : _impl(new MergerImpl) {}

  void Merger::addSource(DiskRunSPtr source)
  {
    _impl->addSource(source);
  }
  
  class DiskRunWriter : public MergeWriter {
  public:
    DiskRunWriter(DiskRunSPtr target)
      : _target(target) {}
    // default dtor/copy/assign OK
    void writeFrom(const DiskRun* run)
    {
      _target->copyCurrentFrom(*run);
    }
  private:
    DiskRunSPtr _target;
  };

  void Merger::merge(DiskRunSPtr target)
  {
    DiskRunWriter writer(target);
    _impl->merge(writer);
  }

  class ReceiverWriter : public MergeWriter {
  public:
    ReceiverWriter(Receiver* target)
      : _target(target) {}
    // default dtor/copy/assign OK
    void writeFrom(const DiskRun* run)
    {
      DiskRun::Item payload = run->getPayload();
      _target->receive(payload._data, payload._length);
    }
  private:
    Receiver* _target;
  };

  void Merger::merge(Receiver* target)
  {
    ReceiverWriter writer(target);
    _impl->merge(writer);
  }

} // namespace external_sort
