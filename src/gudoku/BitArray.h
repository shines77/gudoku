
#ifndef GUDOKU_BITARRAY_H
#define GUDOKU_BITARRAY_H

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
#include <bitset>
#include <cstring>          // For std::memset()
#include <type_traits>
#include <algorithm>        // For std::min()

#include "gudoku/BitSet.h"
#include "gudoku/PackedBitSet.h"

namespace gudoku {

template <size_t Rows, size_t Cols, typename TBitSet = std::bitset<Cols>>
class SmallBitArray2 {
public:
    typedef TBitSet                                 bitset_type;
    typedef SmallBitArray2<Rows, Cols, TBitSet>     this_type;

private:
    bitset_type array_[Rows];

public:
    SmallBitArray2() = default;
    SmallBitArray2(const this_type & src) {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row] = src[row];
        }
    }
    ~SmallBitArray2() = default;

    this_type & operator = (const this_type & right) noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row] = right[row];
        }
        return (*this);
    }

    size_t rows() const { return Rows; }
    size_t cols() const { return Cols; }

    size_t size() const { return Rows; }
    size_t total_size() const { return (Rows * Cols); }

    bool test(size_t row, size_t col) {
        assert(row < Rows);
        return this->array_[row].test(col);
    }

    this_type & fill(size_t value) noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row].fill(value);
        }
        return (*this);
    }

    this_type & set() noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row].set();
        }
        return (*this);
    }

    this_type & reset() noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row].reset();
        }
        return (*this);
    }

    this_type & flip() noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row].flip();
        }
        return (*this);
    }

    bitset_type & operator [] (size_t pos) {
        assert(pos < Rows);
        return this->array_[pos];
    }

    const bitset_type & operator [] (size_t pos) const {
        assert(pos < Rows);
        return this->array_[pos];
    }

    this_type & operator & (const this_type & right) noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row] &= right[row];
        }
        return (*this);
    }

    this_type & operator | (const this_type & right) noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row] |= right[row];
        }
        return (*this);
    }

    this_type & operator ^ (const this_type & right) noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row] ^= right[row];
        }
        return (*this);
    }

    this_type & operator &= (const this_type & right) noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row] &= right[row];
        }
        return (*this);
    }

    this_type & operator |= (const this_type & right) noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row] |= right[row];
        }
        return (*this);
    }

    this_type & operator ^= (const this_type & right) noexcept {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row] ^= right[row];
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
        for (size_t row = 0; row < Rows; row++) {
            if (this->array_[row] != right[row]) {
                return false;
            }
        }
        return true;
    }

    bool operator != (const this_type & right) noexcept {
        for (size_t row = 0; row < Rows; row++) {
            if (this->array_[row] != right[row]) {
                return true;
            }
        }
        return false;
    }
};

template <size_t Depths, size_t Rows, size_t Cols,
          typename TSmallBitArray2 = SmallBitArray2<Rows, Cols, std::bitset<Cols>>>
class SmallBitArray3 {
public:
    typedef TSmallBitArray2                                         array_type;
    typedef typename TSmallBitArray2::bitset_type                   bitset_type;
    typedef SmallBitArray3<Depths, Rows, Cols, TSmallBitArray2>     this_type;

private:
    array_type array_[Depths];

public:
    SmallBitArray3() = default;
    SmallBitArray3(const this_type & src) {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth] = src[depth];
        }
    }
    ~SmallBitArray3() = default;

    this_type & operator = (const this_type & right) noexcept {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth] = right[depth];
        }
        return (*this);
    }

    size_t depths() const { return Depths; }
    size_t rows() const { return Rows; }
    size_t cols() const { return Cols; }

    size_t size() const { return Depths; }
    size_t array2d_size() const { return (Rows * Cols); }
    size_t total_size() const { return (Depths * Rows * Cols); }

    bool test(size_t depth, size_t row, size_t col) {
        assert(depth < Depths);
        return this->array_[depth][row].test(col);
    }

    this_type & fill(size_t value) {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth].fill(value);
        }
        return (*this);
    }

    this_type & set() {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth].set();
        }
        return (*this);
    }

    this_type & reset() {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth].reset();
        }
        return (*this);
    }

    this_type & flip() {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth].flip();
        }
        return (*this);
    }

    array_type & operator [] (size_t pos) {
        assert(pos < Depths);
        return this->array_[pos];
    }

    const array_type & operator [] (size_t pos) const {
        assert(pos < Depths);
        return this->array_[pos];
    }

    this_type & operator & (const this_type & right) noexcept {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth] &= right[depth];
        }
        return (*this);
    }

    this_type & operator | (const this_type & right) noexcept {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth] |= right[depth];
        }
        return (*this);
    }

    this_type & operator ^ (const this_type & right) noexcept {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth] ^= right[depth];
        }
        return (*this);
    }

    this_type & operator &= (const this_type & right) noexcept {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth] &= right[depth];
        }
        return (*this);
    }

    this_type & operator |= (const this_type & right) noexcept {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth] |= right[depth];
        }
        return (*this);
    }

    this_type & operator ^= (const this_type & right) noexcept {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth] ^= right[depth];
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
        for (size_t depth = 0; depth < Depths; depth++) {
            if (this->array_[depth] != right[depth]) {
                return false;
            }
        }
        return true;
    }

    bool operator != (const this_type & right) noexcept {
        for (size_t depth = 0; depth < Depths; depth++) {
            if (this->array_[depth] != right[depth]) {
                return true;
            }
        }
        return false;
    }
};

//
// Array of SmallBitSet
//
template <size_t Rows, size_t Cols>
using SmallBitSet2D = SmallBitArray2<Rows, Cols, SmallBitSet<Cols>>;

template <size_t Depths, size_t Rows, size_t Cols>
using SmallBitSet3D = SmallBitArray3<Depths, Rows, Cols, SmallBitArray2<Rows, Cols, SmallBitSet<Cols>>>;

//
// Array of PackedBitSet
//
template <size_t Rows, size_t Cols>
using PackedBitSet2D = SmallBitArray2<Rows, Cols, PackedBitSet<Cols>>;

template <size_t Depths, size_t Rows, size_t Cols>
using PackedBitSet3D = SmallBitArray3<Depths, Rows, Cols, SmallBitArray2<Rows, Cols, PackedBitSet<Cols>>>;

// BitArray2
template <size_t Rows, size_t Cols, typename TBitSet = std::bitset<Cols>>
class BitArray2 {
public:
    typedef TBitSet                             bitset_type;
    typedef BitArray2<Rows, Cols, TBitSet>      this_type;

private:
    std::vector<bitset_type> array_;

public:
    BitArray2() {
        this->array_.resize(Rows);
    }

    BitArray2(const BitArray2 & src) {
        this->array_.reserve(Rows);
        for (size_t row = 0; row < Rows; row++) {
            this->array_.push_back(src[row]);
        }
    }

    BitArray2(const SmallBitArray2<Rows, Cols, TBitSet> & src) {
        this->array_.reserve(Rows);
        for (size_t row = 0; row < Rows; row++) {
            this->array_.push_back(src[row]);
        }
    }

    ~BitArray2() = default;

    this_type & operator = (const this_type & rhs) {
        if (&rhs != this) {
            for (size_t row = 0; row < Rows; row++) {
                this->array_[row] = rhs[row];
            }
        }
    }

    this_type & operator = (const SmallBitArray2<Rows, Cols, TBitSet> & rhs) {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row] = rhs[row];
        }
    }

    size_t rows() const { return Rows; }
    size_t cols() const { return Cols; }

    size_t size() const { return Rows; }
    size_t total_size() const { return (Rows * Cols); }

    bool test(size_t row, size_t col) {
        assert(row < Rows);
        return this->array_[row].test(col);
    }

    this_type & fill(size_t value) {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row].fill(value);
        }
        return (*this);
    }

    this_type & set() {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row].set();
        }
        return (*this);
    }

    this_type & reset() {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row].reset();
        }
        return (*this);
    }

    this_type & flip() {
        for (size_t row = 0; row < Rows; row++) {
            this->array_[row].flip();
        }
        return (*this);
    }

    bitset_type & operator [] (size_t pos) {
        assert(pos < Rows);
        return this->array_[pos];
    }

    const bitset_type & operator [] (size_t pos) const {
        assert(pos < Rows);
        return this->array_[pos];
    }
};

template <size_t Depths, size_t Rows, size_t Cols,
          typename TBitArray2 = BitArray2<Rows, Cols>>
class BitArray3 {
public:
    typedef TBitArray2                                  array_type;
    typedef typename TBitArray2::bitset_type            bitset_type;
    typedef BitArray3<Depths, Rows, Cols, TBitArray2>   this_type;

private:
    std::vector<array_type> array_;

public:
    BitArray3() {
        this->array_.resize(Depths);
    }

    BitArray3(const this_type & src) {
        this->array_.reserve(Depths);
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_.push_back(src[depth]);
        }
    }

    BitArray3(const SmallBitArray3<Depths, Rows, Cols,
                    SmallBitArray2<Rows, Cols, bitset_type>> & src) {
        this->array_.reserve(Depths);
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_.push_back(src[depth]);
        }
    }

    ~BitArray3() = default;

    BitArray3 & operator = (const this_type & rhs) {
        if (&rhs != this) {
            for (size_t depth = 0; depth < Depths; depth++) {
                this->array_[depth] = rhs[depth];
            }
        }
    }

    BitArray3 & operator = (const SmallBitArray3<Depths, Rows, Cols,
                                  SmallBitArray2<Rows, Cols, bitset_type>> & rhs) {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth] = rhs[depth];
        }
    }

    size_t depths() const { return Depths; }
    size_t rows() const { return Rows; }
    size_t cols() const { return Cols; }

    size_t size() const { return Depths; }
    size_t matrix2d_size() const { return (Rows * Cols); }
    size_t total_size() const { return (Depths * Rows * Cols); }

    bool test(size_t depth, size_t row, size_t col) {
        assert(depth < Depths);
        return this->array_[depth][row].test(col);
    }

    this_type & fill(size_t value) {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth].fill(value);
        }
        return (*this);
    }

    this_type & set() {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth].set();
        }
        return (*this);
    }

    this_type & reset() {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth].reset();
        }
        return (*this);
    }

    this_type & flip() {
        for (size_t depth = 0; depth < Depths; depth++) {
            this->array_[depth].flip();
        }
        return (*this);
    }

    array_type & operator [] (size_t pos) {
        assert(pos < Depths);
        return this->array_[pos];
    }

    const array_type & operator [] (size_t pos) const {
        assert(pos < Depths);
        return this->array_[pos];
    }
};

template <size_t Rows, size_t Cols, typename TBitSet = std::bitset<Cols>>
static void array2d_copy(SmallBitArray2<Rows, Cols, TBitSet> & dest,
                         const BitArray2<Rows, Cols, TBitSet> & src)
{
    for (size_t row = 0; row < Rows; row++) {
        dest[row] = src[row];
    }
}

template <size_t Rows, size_t Cols, typename TBitSet = std::bitset<Cols>>
static void array2d_copy(BitArray2<Rows, Cols, TBitSet> & dest,
                         const SmallBitArray2<Rows, Cols, TBitSet> & src)
{
    for (size_t row = 0; row < Rows; row++) {
        dest[row] = src[row];
    }
}

template <size_t Depths, size_t Rows, size_t Cols,
          typename TSmallBitArray2 = SmallBitArray2<Rows, Cols>,
          typename TBitArray2 = BitArray2<Rows, Cols>>
static void array3d_copy(SmallBitArray3<Depths, Rows, Cols, TSmallBitArray2> & dest,
                         const BitArray3<Depths, Rows, Cols, TBitArray2> & src)
{
    for (size_t depth = 0; depth < Depths; depth++) {
        for (size_t row = 0; row < Rows; row++) {
            dest[depth][row] = src[depth][row];
        }
    }
}

template <size_t Depths, size_t Rows, size_t Cols,
          typename TSmallBitArray2 = SmallBitArray2<Rows, Cols>,
          typename TBitArray2 = BitArray2<Rows, Cols>>
static void array3d_copy(BitArray3<Depths, Rows, Cols, TBitArray2> & dest,
                         const SmallBitArray3<Depths, Rows, Cols, TSmallBitArray2> & src)
{
    for (size_t depth = 0; depth < Depths; depth++) {
        for (size_t row = 0; row < Rows; row++) {
            dest[depth][row] = src[depth][row];
        }
    }
}

} // namespace gudoku

#endif // GUDOKU_BITARRAY_H
