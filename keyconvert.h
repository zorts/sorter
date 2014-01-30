// -*- mode: c++ -*-

// Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
// for the detailed license.

#ifndef EXTERNAL_SORT_KEYCONVERT_H
#define EXTERNAL_SORT_KEYCONVERT_H

#include <stdint.h>

namespace external_sort {
  /*
    The Sorter class treats keys as variable-length byte strings. This
    means that most values must be converted into a form that makes them
    comparable using memcmp. For byte strings and for 8-bit character sets 
    to be compared in byte-value order, no conversion is needed. For other
    types, such as integers and floats, the essence of the conversion is 
    to store them in big-endian order.

    Note that composite keys containing variable-length values (strings,
    byte strings) require additional handling, as it's not possible to
    simply concatenate the values.

    These routines presume:
  
    * little-endian machine
    * "target" pointer points to a large enough buffer
    * source and target are non-overlapping

    Note that this kind of code can be endlessly tweaked for specific platforms;
    this code is deliberately simple to illustrate what has to be done (and to 
    perhaps enable the compiler to Do The Right Thing...)
  */

  // Unsigned integers are easy - just store in big-endian byte order.

  static inline
  void uint8ToKey(uint8_t source, void* target)
  {
    uint8_t* bytes = (uint8_t*) target;
    bytes[0] = source;
  }

  static inline
  void uint16ToKey(uint16_t source, void* target)
  {
    uint8_t* bytes = (uint8_t*) target;
    bytes[0] = (source >> 8) & 0xff; 
    bytes[1] = source & 0xff;
  }

  static inline
  void uint32ToKey(uint32_t source, void* target)
  {
    uint8_t* bytes = (uint8_t*) target;
    bytes[0] = (source >> 24) & 0xff; 
    bytes[1] = (source >> 16) & 0xff; 
    bytes[2] = (source >> 8) & 0xff; 
    bytes[3] = source & 0xff;
  }

  static inline
  void uint64ToKey(uint64_t source, void* target)
  {
    uint8_t* bytes = (uint8_t*) target;
    bytes[0] = (source >> 56) & 0xff; 
    bytes[1] = (source >> 48) & 0xff; 
    bytes[2] = (source >> 40) & 0xff; 
    bytes[3] = (source >> 32) & 0xff; 
    bytes[4] = (source >> 24) & 0xff; 
    bytes[5] = (source >> 16) & 0xff; 
    bytes[6] = (source >> 8) & 0xff; 
    bytes[7] = source & 0xff;
  }

  // Signed integers are slightly trickier - store in big-endian
  // byte order, but with the sign bit inverted.

  static inline
  void int8ToKey(int8_t source, void* target)
  {
    uint8_t temp = (uint8_t) source;
    uint8_t* bytes = (uint8_t*) target;
    bytes[0] = temp ^ 0x80;
  }

  static inline
  void int16ToKey(int16_t source, void* target)
  {
    uint16_t temp = (uint16_t) source;
    uint8_t* bytes = (uint8_t*) target;
    bytes[0] = ((temp >> 8) & 0xff) ^ 0x80; 
    bytes[1] = temp & 0xff;
  }

  static inline
  void int32ToKey(int32_t source, void* target)
  {
    uint32_t temp = (uint32_t)source;
    uint8_t* bytes = (uint8_t*) target;
    bytes[0] = ((temp >> 24) & 0xff) ^ 0x80; 
    bytes[1] = (temp >> 16) & 0xff; 
    bytes[2] = (temp >> 8) & 0xff; 
    bytes[3] = temp & 0xff;
  }

  static inline
  void int64ToKey(int64_t source, void* target)
  {
    uint64_t temp = (uint64_t) source;
    uint8_t* bytes = (uint8_t*) target;
    bytes[0] = ((temp >> 56) & 0xff) ^ 0x80; 
    bytes[1] = (temp >> 48) & 0xff; 
    bytes[2] = (temp >> 40) & 0xff; 
    bytes[3] = (temp >> 32) & 0xff; 
    bytes[4] = (temp >> 24) & 0xff; 
    bytes[5] = (temp >> 16) & 0xff; 
    bytes[6] = (temp >> 8) & 0xff; 
    bytes[7] = temp & 0xff;
  }

  // Floating point is trickier still. I figured this out a long time ago
  // and why it works is left as an exercise for the reader. :-)

  static inline
  void floatToKey(float source, void* target)
  {
    union {
      float _single; // stored little-endian
      uint8_t _bytes[4];
    } raw;
    raw._single = source;
    uint8_t* bytes = (uint8_t*) target;

    if (raw._bytes[3] & 0x80)
    {
      // the sign is negative; invert all bits
      bytes[0] = raw._bytes[3] ^ 0xff; 
      bytes[1] = raw._bytes[2] ^ 0xff; 
      bytes[2] = raw._bytes[1] ^ 0xff; 
      bytes[3] = raw._bytes[0] ^ 0xff; 
    }
    else
    {
      // the sign is positive; just invert the sign bit
      bytes[0] = (raw._bytes[3] & 0xff) ^ 0x80; 
      bytes[1] = raw._bytes[2];
      bytes[2] = raw._bytes[1];
      bytes[3] = raw._bytes[0];
    }
  }

  static inline
  void doubleToKey(double source, void* target)
  {
    union {
      double _double; // stored little-endian
      uint8_t _bytes[8];
    } raw;
    raw._double = source;
    uint8_t* bytes = (uint8_t*) target;

    if (raw._bytes[7] & 0x80)
    {
      // the sign is negative; invert all bits
      bytes[0] = raw._bytes[7] ^ 0xff; 
      bytes[1] = raw._bytes[6] ^ 0xff; 
      bytes[2] = raw._bytes[5] ^ 0xff; 
      bytes[3] = raw._bytes[4] ^ 0xff; 
      bytes[4] = raw._bytes[3] ^ 0xff; 
      bytes[5] = raw._bytes[2] ^ 0xff; 
      bytes[6] = raw._bytes[1] ^ 0xff; 
      bytes[7] = raw._bytes[0] ^ 0xff; 
    }
    else
    {
      // the sign is positive; just invert the sign bit
      bytes[0] = (raw._bytes[7] & 0xff) ^ 0x80; 
      bytes[1] = raw._bytes[6];
      bytes[2] = raw._bytes[5];
      bytes[3] = raw._bytes[4];
      bytes[4] = raw._bytes[3];
      bytes[5] = raw._bytes[2];
      bytes[6] = raw._bytes[1];
      bytes[7] = raw._bytes[0];
    }
  }
}
#endif // EXTERNAL_SORT_KEYCONVERT_H
