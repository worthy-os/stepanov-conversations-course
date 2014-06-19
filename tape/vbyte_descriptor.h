#ifndef VBYTE_DESCRIPTOR_H
#define VBYTE_DESCRIPTOR_H

#include <stdint.h>
#include <stddef.h>
#include <iterator>
#include <utility>

// Only included for "concepts"
#include "variable_size_type.h"

struct vbyte_descriptor {
  typedef uint64_t value_type;
  enum { equality_preserving = true };
  enum { order_preserving = false };
  enum { prefixed_size = false };
  typedef std::bidirectional_iterator_tag iterator_category;

  uint8_t* encode(value_type x, uint8_t* dst) const {
    uint8_t tmp;
    while (x & ~value_type(0x7f)) {
      tmp = (x & 0x7f) | 0x80;
      *dst++ = tmp;
      x >>= 7;
    }
    *dst++ = x;
    return dst;
  }

  // we assume that all the byte sequences passed to these functions are well-formed;
  // a well-formed sequence of bytes is the one which is generated by the encode function
  // for some value of uint64_t

  size_t encoded_size(value_type value) const {
    const value_type one(1);
    if (value < (one << 7))       return size_t(1);
    if (value < (one << 14))      return size_t(2);
    if (value < (one << 21))      return size_t(3);
    if (value < (one << 28))      return size_t(4);
    if (value < (one << 35))      return size_t(5);
    if (value < (one << 42))      return size_t(6);
    if (value < (one << 49))      return size_t(7);
    if (value < (one << 56))      return size_t(8);
    if (value < (one << 63))      return size_t(9);
                                  return size_t(10);
  }
  // 10*7 > 64 > 9*7

  value_type decode(const uint8_t* p) const {
    value_type result = *p++;
    // we special case the frequent case of one-byte values
    if (result < 0x80) return result;
    result &= 0x7f;
    int shift = 7;
    value_type next = *p;
    while (next >= 0x80) {
      result |= (next & 0x7f) << shift;
      shift += 7;  // for well-formed sequences shift < 64
      next = *++p;
    }
    return result | (next << shift);
  }

  size_t size(const uint8_t* p) const {
    size_t result(1);
    while (*p >= 0x80) {
      ++result;
      ++p;
    }
    return result;
  }

  // attributes is almost identical to decode
  // the only difference is the return of the size
  std::pair<value_type, size_t> attributes(const uint8_t* p) const {
    const uint8_t* initial_p = p;
    value_type result = *p++;
    if (result < 0x80) return std::make_pair(result, 1);
    result &= 0x7f;
    uint32_t shift = 7;
    value_type next = *p++;
     while (next >= 0x80) {
      result |= ((next & 0x7f) << shift) ;
      shift += 7;
      next = *p++;
     }
     return std::make_pair(result | (next << shift), p - initial_p);
  }

  // we need to know the beginning of the encoded stream
  // since we search for the previous byte without a continuation bit
  // but need to be guarded if it does not exist
  // as usually done with iterators we assume that the range [origin, current) is valid
  // (pointers are iterators, after all)

  const uint8_t* previous(const uint8_t* origin,
                          const uint8_t* current) const {
    if (current == origin) return current;
    --current;
    while (current != origin) {
      if (*--current < 0x80) return current + 1;
    }
    return current;
  }  

  std::pair<value_type, size_t> attributes_backward(const uint8_t* origin,
                                                    const uint8_t* current) const {
    if (current == origin) return std::make_pair(value_type(0), size_t(0));
    const uint8_t* p = current - 1;
    value_type result = *p;
    while (p != origin) {
      value_type byte = *--p;
      if (byte < 0x80) return std::make_pair(result, current - (p + 1));
      result = (result << 7) | (byte & 0x7f);
    }
    return std::make_pair(result, current - p);
  }  

  // we have the copy function for the sake of completeness;
  // it allows a client to copy a single encoded value
  // encoded integers can be copied as a group using std::copy(const uint8_t*, const uint8_t*, uint8_t*)
  // or, memcopy
  std::pair<const uint8_t*, uint8_t*> 
  copy(const uint8_t* src, uint8_t* dst) const {
    uint8_t value;
    do {
      value = *src++;
      *dst++ = value;
    } while (value & 0x80);
    return std::make_pair(src, dst);
  }
};

// Local Variables:
// mode: c++
// c-basic-offset: 2
// indent-tabs-mode: nil
// End:
#endif