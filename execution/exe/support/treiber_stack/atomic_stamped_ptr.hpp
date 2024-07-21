#pragma once

#include <twist/ed/stdlike/atomic.hpp>

#include <cassert>

//////////////////////////////////////////////////////////////////////

// Notation:
// Raw pointer = T*
// Stamp - unsigned integer
// Stamped pointer = Raw Pointer + Stamp
// Packed stamped pointer = uintptr_t

//////////////////////////////////////////////////////////////////////

// StampedPtr = Raw pointer + Stamp

template <typename T>
struct StampedPtr {
  T* raw_ptr;
  uint64_t stamp;

  StampedPtr(T* p, uint64_t s)
      : raw_ptr(p),
        stamp(s) {
    //
  }

  T* operator->() const {
    return raw_ptr;
  }

  T& operator*() const {
    return *raw_ptr;
  }

  explicit operator bool() const {
    return raw_ptr != nullptr;
  }

  StampedPtr IncrementStamp() const {
    return StampedPtr{raw_ptr, stamp + 1};
  }

  StampedPtr DecrementStamp() const {
    assert(stamp > 0);
    return StampedPtr{raw_ptr, stamp - 1};
  }
};

//////////////////////////////////////////////////////////////////////

template <typename T>
bool operator==(const StampedPtr<T>& lhs, const StampedPtr<T>& rhs) {
  return (lhs.raw_ptr == rhs.raw_ptr) && (lhs.stamp == rhs.stamp);
}

template <typename T>
bool operator!=(const StampedPtr<T>& lhs, const StampedPtr<T>& rhs) {
  return !(lhs == rhs);
}

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
struct Packer {
  static const size_t kFreeBits = 16;
  static const size_t kFreeBitsShift = 64 - kFreeBits;
  static const uintptr_t kFreeBitsMask = (uintptr_t)((1 << kFreeBits) - 1)
                                         << kFreeBitsShift;

  using PackedStampedPtr = uintptr_t;

  static const size_t kMaxStamp = (1 << kFreeBits) - 1;

  static PackedStampedPtr Pack(StampedPtr<T> stamped_ptr) {
    return ClearFreeBits((uintptr_t)stamped_ptr.raw_ptr) |
           (stamped_ptr.stamp << kFreeBitsShift);
  }

  static StampedPtr<T> Unpack(PackedStampedPtr packed_ptr) {
    return {GetRawPointer(packed_ptr), GetStamp(packed_ptr)};
  }

 private:
  static int GetBit(uintptr_t mask, size_t index) {
    return (mask >> index) & 1;
  }

  static uintptr_t SetFreeBits(uintptr_t mask) {
    return mask | kFreeBitsMask;
  }

  static uintptr_t ClearFreeBits(uintptr_t mask) {
    return mask & ~kFreeBitsMask;
  }

  // https://en.wikipedia.org/wiki/X86-64#Canonical_form_addresses
  static uintptr_t ToCanonicalForm(uintptr_t ptr) {
    if (GetBit(ptr, 47) != 0) {
      return SetFreeBits(ptr);
    } else {
      return ClearFreeBits(ptr);
    }
  }

  static uint64_t GetStamp(PackedStampedPtr packed_ptr) {
    return packed_ptr >> kFreeBitsShift;
  }

  static T* GetRawPointer(PackedStampedPtr packed_ptr) {
    return (T*)ToCanonicalForm(packed_ptr);
  }
};

}  // namespace detail

//////////////////////////////////////////////////////////////////////

template <typename T>
using PackedStampedPtr = typename detail::Packer<T>::PackedStampedPtr;

template <typename T>
PackedStampedPtr<T> Pack(StampedPtr<T> ptr) {
  return detail::Packer<T>::Pack(ptr);
}

template <typename T>
StampedPtr<T> Unpack(PackedStampedPtr<T> ptr) {
  return detail::Packer<T>::Unpack(ptr);
}

//////////////////////////////////////////////////////////////////////

// 48-bit pointer + 16-bit stamp packed into single 64-bit word

// Usage:

// asp.Store({raw_ptr, 42});
// auto ptr = asp.Load();
// if (asp) { ... }
// asp->Foo();
// asp.CompareExchangeWeak(expected_stamped_ptr, {raw_ptr, 42});

// auto e =
// top_.compare_exchange(target.Pack(),

template <typename T>
class AtomicStampedPtr {
  using Packer = detail::Packer<T>;
  using PackedStampedPtr = typename Packer::PackedStampedPtr;

 public:
  static const size_t kMaxStamp = Packer::kMaxStamp;

 public:
  explicit AtomicStampedPtr(StampedPtr<T> ptr)
      : packed_ptr_(Pack(ptr)) {
  }

  void Store(StampedPtr<T> ptr,
             std::memory_order mo = std::memory_order::seq_cst) {
    packed_ptr_.store(Pack(ptr), mo);
  }

  StampedPtr<T> Load(std::memory_order mo = std::memory_order::seq_cst) const {
    return Unpack(packed_ptr_.load(mo));
  }

  StampedPtr<T> Exchange(
      StampedPtr<T> target, std::memory_order mo = std::memory_order::seq_cst) {
    PackedStampedPtr old_packed = packed_ptr_.exchange(Pack(target), mo);
    return Unpack(old_packed);
  }

  bool CompareExchangeWeak(
      StampedPtr<T>& expected, StampedPtr<T> desired, std::memory_order success,
      std::memory_order failure) {
    return CompareExchangeWeakImpl(expected, desired, success, failure);
  }

  bool CompareExchangeWeak(
      StampedPtr<T>& expected, StampedPtr<T> desired,
      std::memory_order mo = std::memory_order::seq_cst) {
    return CompareExchangeWeakImpl(expected, desired, mo, mo);
  }

 private:
  bool CompareExchangeWeakImpl(StampedPtr<T>& expected, StampedPtr<T> desired,
                               std::memory_order success,
                               std::memory_order failure) {
    PackedStampedPtr expected_packed = Pack(expected);
    bool succeeded = packed_ptr_.compare_exchange_weak(
        expected_packed, Pack(desired), success, failure);
    if (!succeeded) {
      expected = Unpack(expected_packed);
    }
    return succeeded;
  }

 private:
  static PackedStampedPtr Pack(StampedPtr<T> ptr) {
    return Packer::Pack(ptr);
  }

  static StampedPtr<T> Unpack(PackedStampedPtr packed_ptr) {
    return Packer::Unpack(packed_ptr);
  }

 private:
  twist::ed::stdlike::atomic<PackedStampedPtr> packed_ptr_;
};
