
#ifndef GUDOKU_PACKED_BITSET_H
#define GUDOKU_PACKED_BITSET_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstring>          // For std::memset()
#include <initializer_list>
#include <type_traits>
#include <algorithm>        // For std::min()
#include <cassert>

#include "gudoku/BitUtils.h"
#include "gudoku/BitSet.h"
#include "gudoku/PowerOf2.h"

namespace gudoku {

template <size_t Bytes>
struct IntegerType {
    typedef size_t type;
};

template <>
struct IntegerType<0U> {
    typedef uint8_t type;
};

template <>
struct IntegerType<1U> {
    typedef uint8_t type;
};

template <>
struct IntegerType<2U> {
    typedef uint16_t type;
};

template <>
struct IntegerType<3U> {
    typedef uint32_t type;
};

template <>
struct IntegerType<4U> {
    typedef uint32_t type;
};

template <>
struct IntegerType<5U> {
    typedef uint64_t type;
};

template <>
struct IntegerType<6U> {
    typedef uint64_t type;
};

template <>
struct IntegerType<7U> {
    typedef uint64_t type;
};

template <>
struct IntegerType<8U> {
    typedef uint64_t type;
};

template <size_t N>
struct RoundToPow2 {
    static const size_t value = compile_time::round_to_pow2<N>::value;
};

#pragma pack(push, 1)

template <size_t Bits>
class PackedBitSet {
public:
    static const size_t kByteBits = 8;
    static const size_t kSizeOfLong = sizeof(size_t);
    static const size_t kAlignmentBits = sizeof(uint8_t) * kByteBits;
    static const size_t kMaxAlignmentBits = kSizeOfLong * kByteBits;
    static const size_t kUnitAlignmentBits = (Bits < kMaxAlignmentBits) ?
                                         RoundToPow2<Bits>::value :
                                         kMaxAlignmentBits;

    static const size_t kUnitBits  = kUnitAlignmentBits;
    static const size_t kUnitBytes = kUnitAlignmentBits / kByteBits;

    typedef typename IntegerType<kUnitBytes>::type  unit_type;
    typedef PackedBitSet<Bits>                      this_type;

    static const size_t kUnits = (Bits + kUnitBits - 1) / kUnitBits;
    static const size_t kBits  = kUnits * kUnitBits;
    static const size_t kBytes = kUnits * kUnitBytes;
    static const size_t kRestBits = (Bits % kUnitBits);
    static const unit_type kFullMask = unit_type(-1);
    static const unit_type kTrimMask = (kRestBits != 0) ? (unit_type(size_t(1) << kRestBits) - 1) : kFullMask;

    static const size_t kMemSetThresholdUnits = 4;

private:
    unit_type array_[kUnits];

public:
    PackedBitSet() noexcept {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        this->reset();
    }

    PackedBitSet(dont_init_t & dont_init) noexcept {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        /* Here we don't need initialize for optimize sometimes. */
    }

    PackedBitSet(const this_type & src) noexcept {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] = src.array(i);
        }
    }

    PackedBitSet(unit_type value) noexcept {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        if (kRestBits == 0)
            this->array_[0] = value;
        else
            this->array_[0] = value & kTrimMask;
    }

    PackedBitSet(std::initializer_list<unit_type> init_list) noexcept {
        static_assert((Bits != 0), "PackedBitSet<Bits>: Bits can not be 0 size.");
        if (init_list.size() <= kUnits) {
            size_t i = 0;
            for (auto iter : init_list) {
                this->array_[i++] = *iter;
            }
            if (kRestBits != 0) {
                this->trim();
            }
        }
        else {
            size_t i = 0;
            for (auto iter : init_list) {
                this->array_[i++] = *iter;
                if (i >= kUnits) {
                    break;
                }
            }
            if (kRestBits != 0) {
                this->trim();
            }
        }
    }

    ~PackedBitSet() = default;

    size_t size() const        { return Bits; }

          char * data()        { return (char *)      this->array_; }
    const char * data() const  { return (const char *)this->array_; }

    size_t total_bytes() const { return kBytes;     }
    size_t unit_size() const   { return kUnits;     }
    size_t unit_bits() const   { return kUnitBits;  }
    size_t unit_bytes() const  { return kUnitBytes; }

    unit_type array(size_t index) const {
        assert(index < kUnits);
        return this->array_[index];
    }

    this_type & init(std::initializer_list<unit_type> init_list) noexcept {
        if (init_list.size() <= kUnits) {
            size_t i = 0;
            for (auto iter : init_list) {
                this->array_[i++] = *iter;
            }
            if (kRestBits != 0) {
                this->trim();
            }
        }
        else {
            size_t i = 0;
            for (auto iter : init_list) {
                this->array_[i++] = *iter;
                if (i >= kUnits) {
                    break;
                }
            }
            if (kRestBits != 0) {
                this->trim();
            }
        }
        return (*this);
    }

    class reference {
    private:
        this_type * bitset_;    // pointer to the bitmap
        size_t pos_;            // position of element in bitset

        // proxy for an element
        friend class PackedBitSet<Bits>;

    public:
        ~reference() noexcept {
            // destroy the object
        }

        reference & operator = (bool value) noexcept {
            // assign Boolean to element
            this->bitset_->set(pos_, value);
            return (*this);
        }

        reference & operator = (const reference & right) noexcept {
            // assign reference to element
            this->bitset_->set(pos_, bool(right));
            return (*this);
        }

        reference & flip() noexcept {
            // complement stored element
            this->bitset_->flip(pos_);
            return (*this);
        }

        bool operator ~ () const noexcept {
            // return complemented element
            return (!this->bitset_->test(pos_));
        }

        bool operator ! () const noexcept {
            // return complemented element
            return (!this->bitset_->test(pos_));
        }

        operator bool () const noexcept {
            // return element
            return (this->bitset_->test(pos_));
        }

    private:
        reference() noexcept
            : bitset_(nullptr), pos_(0) {
            // default construct
        }

        reference(this_type & bitsets, size_t pos) noexcept
            : bitset_(&bitsets), pos_(pos) {
            // construct from bitmap reference and position
        }
    };

    this_type & operator = (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] = right.array(i);
        }
        return (*this);
    }

    constexpr bool operator [] (size_t pos) const {
        assert(pos < Bits);
        return this->test(pos);
    }

	reference operator [] (size_t pos) {
        assert(pos < Bits);
        return reference(*this, pos);
    }

    this_type & operator & (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] &= value;
        else
            this->array_[0] &= value & kTrimMask;
        return (*this);
    }

    this_type & operator | (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] |= value;
        else
            this->array_[0] |= value & kTrimMask;
        return (*this);
    }

    this_type & operator ^ (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] ^= value;
        else
            this->array_[0] ^= value & kTrimMask;
        return (*this);
    }

    this_type & operator &= (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] &= value;
        else
            this->array_[0] &= value & kTrimMask;
        return (*this);
    }

    this_type & operator |= (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] |= value;
        else
            this->array_[0] |= value & kTrimMask;
        return (*this);
    }

    this_type & operator ^= (unit_type value) noexcept {
        if (kRestBits == 0)
            this->array_[0] ^= value;
        else
            this->array_[0] ^= value & kTrimMask;
        return (*this);
    }

    this_type & operator &= (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] &= right.array(i);
        }
        return (*this);
    }

    this_type & operator |= (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] |= right.array(i);
        }
        return (*this);
    }

    this_type & operator ^= (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            this->array_[i] ^= right.array(i);
        }
        return (*this);
    }

	this_type operator ~ () const noexcept {
        // Flip all bits
		return (this_type(*this).flip());
    }

	this_type operator ! () const noexcept {
        // Flip all bits
		return (this_type(*this).flip());
    }

    bool operator == (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            if (this->array_[i] != right.array(i)) {
                return false;
            }
        }
        return true;
    }

    bool operator != (const this_type & right) noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            if (this->array_[i] != right.array(i)) {
                return true;
            }
        }
        return false;
    }

    this_type & fill(size_t value) noexcept {
        if (kRestBits != 0) {
            size_t i = 0;
            for (; i < kUnits - 1; i++) {
                this->array_[i] = (unit_type)value;
            }
            this->array_[i] = (unit_type)value & kTrimMask;
        }
        else {
            for (size_t i = 0; i < kUnits; i++) {
                this->array_[i] = (unit_type)value;
            }
        }
        return (*this);
    }

    this_type & set() noexcept {
        if (kRestBits != 0) {
            if (kUnits <= kMemSetThresholdUnits) {
                size_t i = 0;
                for (; i < kUnits - 1; i++) {
                    this->array_[i] = kFullMask;
                }
                this->array_[i] = kTrimMask;
            }
            else {
                std::memset(this->array_, (kFullMask & 0xFF), (kUnits - 1) * sizeof(unit_type));
                this->array_[kUnits - 1] = kTrimMask;
            }
        }
        else {
            if (kUnits <= kMemSetThresholdUnits) {
                for (size_t i = 0; i < kUnits; i++) {
                    this->array_[i] = kFullMask;
                }
            }
            else {
                std::memset(this->array_, (kFullMask & 0xFF), kUnits * sizeof(unit_type));
            }
        }
        return (*this);
    }

    this_type & set(size_t pos) {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            this->array_[0] |= unit_type(size_t(1) << pos);
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            this->array_[index] |= unit_type(size_t(1) << shift);
        }
        return (*this);
    }

    this_type & set(size_t pos, bool value) {
        if (value)
            this->set(pos);
        else
            this->reset(pos);
        return (*this);
    }

    this_type & set(size_t index, size_t pos) {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        this->array_[index] |= unit_type(size_t(1) << pos);
        return (*this);
    }

    this_type & set_bit(size_t index, size_t bit) {
        assert(index < kUnits);
        this->array_[index] |= unit_type(bit);
        return (*this);
    }

    this_type & set(size_t index, size_t pos, bool value) {
        if (value)
            this->set(index, pos);
        else
            this->reset(index, pos);
        return (*this);
    }

    this_type & reset() noexcept {
        if (kUnits <= kMemSetThresholdUnits) {
            for (size_t i = 0; i < kUnits; i++) {
                this->array_[i] = 0;
            }
        }
        else {
            std::memset(this->array_, 0, kUnits * sizeof(unit_type));
        }
        return (*this);
    }

#if 1

    this_type & reset(size_t pos) {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            this->array_[0] ^= unit_type(size_t(1) << pos);
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            this->array_[index] ^= unit_type(size_t(1) << shift);
        }
        return (*this);
    }

    this_type & reset(size_t index, size_t pos) {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        this->array_[index] ^= unit_type(size_t(1) << pos);
        return (*this);
    }

    this_type & reset_bit(size_t index, size_t bit) {
        assert(index < kUnits);
        this->array_[index] ^= unit_type(bit);
        return (*this);
    }

#else

    this_type & reset(size_t pos) {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            this->array_[0] &= unit_type(~(size_t(1) << pos));
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            this->array_[index] &= unit_type(~(size_t(1) << shift));
        }
        return (*this);
    }

    this_type & reset(size_t index, size_t pos) {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        this->array_[index] &= unit_type(~(size_t(1) << pos));
        return (*this);
    }

    this_type & reset_bit(size_t index, size_t bit) {
        assert(index < kUnits);
        this->array_[index] &= unit_type(~bit);
        return (*this);
    }

#endif

    this_type & flip() noexcept {
        if (kRestBits != 0) {
            size_t i = 0;
            for (; i < kUnits - 1; i++) {
                this->array_[i] ^= kFullMask;
            }
            this->array_[i] ^= kTrimMask;
        }
        else {
            for (size_t i = 0; i < kUnits; i++) {
                this->array_[i] ^= kFullMask;
            }
        }
        return (*this);
    }

    this_type & flip(size_t pos) {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            this->array_[0] ^= unit_type(~(size_t(1) << pos));
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            this->array_[index] ^= unit_type(~(size_t(1) << shift));
        }
        return (*this);
    }

    this_type & flip(size_t index, size_t pos) {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        this->array_[index] ^= unit_type(~(size_t(1) << pos));
        return (*this);
    }

    this_type & flip_bit(size_t index, size_t bit) {
        assert(index < kUnits);
        this->array_[index] ^= unit_type(~bit);
        return (*this);
    }

    this_type & trim() noexcept {
        if (kRestBits != 0) {
		    this->array_[kUnits - 1] &= kTrimMask;
        }
        return (*this);
    }

    bool test(size_t pos) const {
        assert(pos < Bits);
        if (Bits <= kUnitBits) {
            return ((this->array_[0] & unit_type(size_t(1) << pos)) != 0);
        }
        else {
            size_t index = pos / kUnitBits;
            size_t shift = pos % kUnitBits;
            return ((this->array_[index] & unit_type(size_t(1) << shift)) != 0);
        }
    }

    bool test(size_t index, size_t pos) const {
        assert(index < kUnits);
        assert(pos < kUnitBits);
        return ((this->array_[index] & unit_type(size_t(1) << pos)) != 0);
    }

    bool test_bit(size_t index, size_t bit) const {
        assert(index < kUnits);
        return ((this->array_[index] & unit_type(bit)) != 0);
    }

    bool any() const noexcept {
        for (size_t i = 0; i < kUnits - 1; i++) {
            unit_type unit = this->array_[i];
            if (unit != 0) {
                return true;
            }
        }
        return (this->array_[kUnits - 1] != 0);
    }

    bool none() const noexcept {
#if 1
        return !(this->any());
#else
        for (size_t i = 0; i < kUnits - 1; i++) {
            unit_type unit = this->array_[i];
            if (unit != 0) {
                return false;
            }
        }
        return (this->array_[kUnits - 1] == 0);
#endif
    }

    bool all() const noexcept {
        for (size_t i = 0; i < kUnits - 1; i++) {
            unit_type unit = this->array_[i];
            if (unit != kFullMask) {
                return false;
            }
        }
        if (kRestBits != 0) {
            unit_type unit = this->array_[kUnits - 1] & kTrimMask;
            return (unit == kTrimMask);
        }
        else {
            return (this->array_[kUnits - 1] == kFullMask);
        }
    }

    size_t bsf() const noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            unit_type unit = this->array_[i];
            if (unit != 0) {
                unsigned int index;
                if (sizeof(unit_type) == sizeof(uint32_t))
                    index = BitUtils::bsf32((uint32_t)unit);
                else
                    index = BitUtils::bsf64(unit);
                return (i * kUnitBits + index);
            }
        }
        return 0;
    }

    size_t bsr() const noexcept {
        for (ptrdiff_t i = kUnits - 1; i >= 0; i--) {
            unit_type unit = this->array_[i];
            if (unit != 0) {
                unsigned int index;
                if (sizeof(unit_type) <= sizeof(uint32_t))
                    index = BitUtils::bsr32((uint32_t)unit);
                else
                    index = BitUtils::bsr64(unit);
                return size_t(i * kUnitBits + index);
            }
        }
        return Bits;
    }

    inline unit_type ls1b() const noexcept {
        unit_type unit = this->array_[0];
        if (unit != 0) {
            unit_type bit;
            if (sizeof(unit_type) <= sizeof(uint32_t))
                bit = (unit_type)BitUtils::ls1b32((uint32_t)unit);
            else
                bit = (unit_type)BitUtils::ls1b64(unit);
            return bit;
        }
        return unit_type(0);
    }

    inline unit_type ls1b(size_t & index) const noexcept {
        for (size_t i = 0; i < kUnits; i++) {
            unit_type unit = this->array_[i];
            if (unit != 0) {
                unit_type bit;
                if (sizeof(unit_type) <= sizeof(uint32_t))
                    bit = (unit_type)BitUtils::ls1b32((uint32_t)unit);
                else
                    bit = (unit_type)BitUtils::ls1b64(unit);
                index = i;
                return bit;
            }
        }
        index = -1;
        return unit_type(0);
    }

    inline size_t count() const noexcept {
        size_t total_popcnt = 0;
        for (size_t i = 0; i < kUnits; i++) {
            size_t unit = this->array_[i];
            unsigned int popcnt = BitUtils::popcnt(unit);
            total_popcnt += popcnt;
        }
        return total_popcnt;
    }

    inline unit_type value(size_t index = 0) const {
        assert(index < kUnits);
        return this->array_[index];
    }

    inline void set_value(unit_type value, size_t index = 0) const {
        assert(index < kUnits);
        return this->array_[index] = value;
    }

    inline unsigned long to_ulong(size_t index = 0) const {
        assert(index < kUnits);
        return static_cast<unsigned long>(this->array_[index]);
    }

    inline uint64_t to_ullong(size_t index = 0) const {
        assert(index < kUnits);
        return static_cast<uint64_t>(this->array_[index]);
    }

    inline size_t reset_and_get(size_t index = 0) {
        assert(index < kUnits);
        size_t value = static_cast<size_t>(this->array_[index]);
        this->array_[index] = 0;
        return value;
    }

    inline size_t set_and_get(size_t value, size_t index = 0) {
        assert(index < kUnits);
        size_t oldValue = static_cast<size_t>(this->array_[index]);
        this->array_[index] = static_cast<unit_type>(value);
        return oldValue;
    }
};

#pragma pack(pop)

template <size_t Bits>
inline
PackedBitSet<Bits> operator & (const PackedBitSet<Bits> & left,
                               const PackedBitSet<Bits> & right) noexcept {
    // left And right
    PackedBitSet<Bits> result = left;
    result &= right;
    return result;
}

template <size_t Bits>
inline
PackedBitSet<Bits> operator | (const PackedBitSet<Bits> & left,
                               const PackedBitSet<Bits> & right) noexcept {
    // left Or right
    PackedBitSet<Bits> result = left;
    result |= right;
    return result;
}

template <size_t Bits>
inline
PackedBitSet<Bits> operator ^ (const PackedBitSet<Bits> & left,
                               const PackedBitSet<Bits> & right) noexcept {
    // left Xor right
    PackedBitSet<Bits> result = left;
    result ^= right;
    return result;
}

} // namespace gudoku

#endif // GUDOKU_PACKED_BITSET_H
