// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#ifndef EXTERNAL_SORT_DISKRUN_H
#define EXTERNAL_SORT_DISKRUN_H

#include <memory>
#include <sys/uio.h> // for iovec

namespace external_sort {

  class DiskRun;
  typedef std::shared_ptr<DiskRun> DiskRunSPtr;

  class DiskRun {
  public:
    ~DiskRun();
    static DiskRunSPtr getDiskRun(unsigned int level, 
                                  unsigned int keyBytes,
                                  unsigned int payloadBytes);

    bool isWritable() const
    {
      return _isWritable;
    }

    void write(const void* key, unsigned int keyLength,
               const void* payload, unsigned int payloadLength);

    void resetForRead();

    struct Item {
      Item()
        : _data(0)
        , _length(0) {}

      Item(const void* data, unsigned int length)
        : _data(data)
        , _length(length) {}

      // default dtor/copy/assign OK
      const void* _data;
      unsigned int _length;
    };

    bool next();
    Item getKey() const;
    Item getPayload() const;
    void copyCurrentFrom(const DiskRun& source);
  private:
    DiskRun();
    // Prohibit copy/assign; do not implement
    DiskRun(const DiskRun&);
    DiskRun& operator=(const DiskRun&);

    struct Header {
      // default ctor/dtor/copy/assign OK
      unsigned int _keyPlusPayloadLength;
      unsigned int _keyLength;
    };

    // Linux implementation
    std::unique_ptr<char> _buffer;
    iovec _writeVector[3];
    Header _header;
    int _fd;
    unsigned int _maxRecordSize;
    bool _isWritable;

    void close();

    static unsigned int _seq;
  };

} // namespace external_sort

#endif // EXTERNAL_SORT_DISKRUN_H

