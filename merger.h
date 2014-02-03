// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#ifndef EXTERNAL_SORT_MERGER_H
#define EXTERNAL_SORT_MERGER_H

#include <vector>
#include <memory>

namespace external_sort {

  class DiskRun;
  typedef std::shared_ptr<DiskRun> DiskRunSPtr;
  class Receiver;
  class MergerImpl;
  typedef std::unique_ptr<MergerImpl> MergerImplSPtr;

  class Merger {
  public:
    Merger();
    // default dtor OK
    void addSource(DiskRunSPtr);
    void merge(DiskRunSPtr);
    void merge(Receiver*);

  private:
    // Prohibit copy/assign; do not implement
    Merger(const Merger&);
    Merger& operator=(const Merger&);
    MergerImplSPtr _impl;
  };

} // namespace external_sort

#endif // EXTERNAL_SORT_MERGER_H

