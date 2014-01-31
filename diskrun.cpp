// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#include "diskrun.h"
#include "sortassert.h"

#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>

// TBD: This initial implementation is drop-dead simple just to get it running.

namespace external_sort {

  unsigned int DiskRun::_seq = 0;

  DiskRun::DiskRun()
    : _buffer(0)
    , _fd(-1)
    , _maxRecordSize(0)
    , _writing(true)
  {
    _writeVector[0].iov_base = &_header;
    _writeVector[0].iov_len = sizeof(_header);
  }

  void DiskRun::close()
  {
    if (_fd != -1)
    {
      // TBD: real error handling
      SORT_ASSERT(0 == ::close(_fd));
      _fd = -1;
    }
  }

  DiskRun::~DiskRun()
  {
    close();
    delete _buffer;
    _buffer = 0;
  }

  DiskRun* DiskRun::getDiskRun(unsigned int level, 
                               unsigned int keyBytes,
                               unsigned int payloadBytes)
  {
    DiskRun* result = new DiskRun;
    // TBD
    std::ostringstream s;
    s << "sort_level_" << level
      << "_key_" << keyBytes
      << "_payload_" << payloadBytes
      << "_XXXXXX"
      ;
    std::string nameTemplate = s.str();
    result->_fd = ::mkstemp(const_cast<char*>(nameTemplate.data()));
    SORT_ASSERT(result->_fd);
    return result;
  }

  void DiskRun::write(const void* key, unsigned int keyLength,
                      const void* payload, unsigned int payloadLength)
  {
    _header._keyPlusPayloadLength = + keyLength + payloadLength;
    _header._keyLength = keyLength;
    if (_header._keyPlusPayloadLength > _maxRecordSize)
    {
      _maxRecordSize = _header._keyPlusPayloadLength;
    }
    _writeVector[1].iov_base = const_cast<void*>(key);
    _writeVector[1].iov_len = (size_t) keyLength;
    _writeVector[2].iov_base = const_cast<void*>(payload);
    _writeVector[2].iov_len = (size_t) payloadLength;
    ssize_t written = ::writev(_fd, _writeVector, 3);
    // TBD: real exceptions
    SORT_ASSERT(written == (ssize_t) _header._keyPlusPayloadLength + sizeof(Header));
  }

  void DiskRun::resetForRead()
  {
    SORT_ASSERT(_writing);
    _writing = false;
    off_t where = ::lseek(_fd, 0, SEEK_SET);
    // TBD: real exception
    SORT_ASSERT(((off_t)0) == where);
    _buffer = new char[_maxRecordSize];
  }

  struct Item {
    const void* _data;
    unsigned int _length;
  };

  bool DiskRun::next()
  {
    SORT_ASSERT_DEBUGONLY(!_writing);
    ssize_t amountRead = read(_fd, &_header, sizeof(Header));
    if (amountRead == sizeof(Header))
    {
      ssize_t dataLength = (size_t) (_header._keyPlusPayloadLength);
      amountRead = read(_fd, _buffer, dataLength);
      if (amountRead == dataLength)
      {
        return true;
      }
      // TBD
      SORT_ASSERT(amountRead == dataLength);
    }

    // TBD
    SORT_ASSERT(amountRead == 0); // EOF
    close();
    return false;
  }

  DiskRun::Item DiskRun::getKey() const
  {
    SORT_ASSERT_DEBUGONLY(!_writing);
    SORT_ASSERT_DEBUGONLY(_fd != -1);
    return Item(_buffer, _header._keyLength);
  }

  DiskRun::Item DiskRun::getPayload() const
  {
    SORT_ASSERT_DEBUGONLY(!_writing);
    SORT_ASSERT_DEBUGONLY(_fd != -1);
    return Item(_buffer+_header._keyLength, _header._keyPlusPayloadLength - _header._keyLength);
  }

} // namespace external_sort
