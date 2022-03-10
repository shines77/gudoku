
#ifndef GUDOKU_SUDOKU_H
#define GUDOKU_SUDOKU_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <bitset>
#include <cstring>      // For std::memset(), std::memcpy()
#include <type_traits>
#include <algorithm>    // For std::sort(), std::fill_n()

#include <vector>
#include <atomic>

#include "gudoku/stddef.h"
#include "gudoku/BitSet.h"
#include "gudoku/BitArray.h"
#include "gudoku/BitVec.h"

namespace gudoku {

struct Status {
    enum {
        Failed = -2,
        Invalid = -1,
        Unsolvable = -1,
        Success = 0,
        Solved = 1,
        UniqueSolution = 2,
        ReachMaxSolutions = 3
    };
};

struct VerifyMode {
    enum {
        VerifyValidity = 0,
        VerifyCorrectness
    };
};

struct SearchMode {
    enum {
        AllSolutions = 0,
        OneSolution = 1,
        MoreThanOneSolution = 2
    };
};

#pragma pack(push, 1)

struct Board {
    char cells[81];

    void clear() {
#if 1
        //std::fill_n(this->cells, sizeof(this->cells), 0);
        std::memset(&this->cells[0], 0, sizeof(this->cells));
#else
        BitVec16x16 zeros;
        zeros.setAllZeros();
        zeros.saveAligned(&this->cells[0]);
        zeros.saveAligned(&this->cells[32]);
        BitVec08x16 zeros_16;
        zeros_16.setAllZeros();
        zeros_16.saveAligned(&this->cells[64]);
        this->cells[81] = 0;
#endif
    }
};

struct alignas(32) Board96 {
    char cells[96];

    void clear() {
#if 0
        //std::fill_n(this->cells, sizeof(this->cells), 0);
        std::memset(&this->cells[0], 0, sizeof(this->cells));
#else
        BitVec16x16 zeros;
        zeros.setAllZeros();
        zeros.saveAligned(&this->cells[0]);
        zeros.saveAligned(&this->cells[32]);
        zeros.saveAligned(&this->cells[64]);
#endif
    }
};

#pragma pack(pop)

template <size_t Size, size_t Alignment>
struct AlignedTo {
    static const size_t alignment = Alignment;
    static const size_t size = Size;
    static const size_t value = (size + alignment - 1) / alignment * alignment;
};

static
double calc_percent(size_t num_val, size_t num_total) {
    if (num_total != 0)
        return (num_val * 100.0) / num_total;
    else
        return 0.0;
}

struct sTables {
    const unsigned int div3[9] = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
    const unsigned int mod3[9] = { 0, 1, 2, 0, 1, 2, 0, 1, 2 };

    const unsigned int roundTo3[9] = { 0, 0, 0, 3, 3, 3, 6, 6, 6 };

    const unsigned int div9[81] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7,
        8, 8, 8, 8, 8, 8, 8, 8, 8
    };

    const unsigned int mod9[81] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        0, 1, 2, 3, 4, 5, 6, 7, 8
    };

    const unsigned int div27[81] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2,
    };

    const unsigned int mod27[81] = {
        0,  1,  2,  3,  4,  5,  6,  7,  8,
        9,  10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26,
        0,  1,  2,  3,  4,  5,  6,  7,  8,
        9,  10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26,
        0,  1,  2,  3,  4,  5,  6,  7,  8,
        9,  10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26,
    };

    const unsigned int posToMask[81] = {
        0x1,        0x2,        0x4,        0x8,        0x10,       0x20,       0x40,       0x80,       0x100,
        0x200,      0x400,      0x800,      0x1000,     0x2000,     0x4000,     0x8000,     0x10000,    0x20000,
        0x40000,    0x80000,    0x100000,   0x200000,   0x400000,   0x800000,   0x1000000,  0x2000000,  0x4000000,
        0x1,        0x2,        0x4,        0x8,        0x10,       0x20,       0x40,       0x80,       0x100,
        0x200,      0x400,      0x800,      0x1000,     0x2000,     0x4000,     0x8000,     0x10000,    0x20000,
        0x40000,    0x80000,    0x100000,   0x200000,   0x400000,   0x800000,   0x1000000,  0x2000000,  0x4000000,
        0x1,        0x2,        0x4,        0x8,        0x10,       0x20,       0x40,       0x80,       0x100,
        0x200,      0x400,      0x800,      0x1000,     0x2000,     0x4000,     0x8000,     0x10000,    0x20000,
        0x40000,    0x80000,    0x100000,   0x200000,   0x400000,   0x800000,   0x1000000,  0x2000000,  0x4000000,
    };

    const unsigned int box[81] = {
        0, 0, 0, 1, 1, 1, 2, 2, 2,
        0, 0, 0, 1, 1, 1, 2, 2, 2,
        0, 0, 0, 1, 1, 1, 2, 2, 2,
        3, 3, 3, 4, 4, 4, 5, 5, 5,
        3, 3, 3, 4, 4, 4, 5, 5, 5,
        3, 3, 3, 4, 4, 4, 5, 5, 5,
        6, 6, 6, 7, 7, 7, 8, 8, 8,
        6, 6, 6, 7, 7, 7, 8, 8, 8,
        6, 6, 6, 7, 7, 7, 8, 8, 8
    };

    const unsigned int box_X_9[81] = {
        0,   0,  0,  9,  9,  9, 18, 18, 18,
        0,   0,  0,  9,  9,  9, 18, 18, 18,
        0,   0,  0,  9,  9,  9, 18, 18, 18,
        27, 27, 27, 36, 36, 36, 45, 45, 45,
        27, 27, 27, 36, 36, 36, 45, 45, 45,
        27, 27, 27, 36, 36, 36, 45, 45, 45,
        54, 54, 54, 63, 63, 63, 72, 72, 72,
        54, 54, 54, 63, 63, 63, 72, 72, 72,
        54, 54, 54, 63, 63, 63, 72, 72, 72
    };

    const unsigned int box_base[81] = {
        0,   0,  0,  3,  3,  3,  6,  6,  6,
        0,   0,  0,  3,  3,  3,  6,  6,  6,
        0,   0,  0,  3,  3,  3,  6,  6,  6,
        27, 27, 27, 30, 30, 30, 33, 33, 33,
        27, 27, 27, 30, 30, 30, 33, 33, 33,
        27, 27, 27, 30, 30, 30, 33, 33, 33,
        54, 54, 54, 57, 57, 57, 60, 60, 60,
        54, 54, 54, 57, 57, 57, 60, 60, 60,
        54, 54, 54, 57, 57, 57, 60, 60, 60
    };

    const uint8_t cell_info[81][10] = {
        { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  // 0
        { 0,  1,  0,  1,  1,  0,  0,  0,  1,  0 },  // 1
        { 0,  2,  0,  2,  2,  0,  0,  0,  2,  0 },  // 2
        { 0,  3,  1,  0,  9,  3,  1,  1,  0,  0 },  // 3
        { 0,  4,  1,  1, 10,  3,  1,  1,  1,  0 },  // 4
        { 0,  5,  1,  2, 11,  3,  1,  1,  2,  0 },  // 5
        { 0,  6,  2,  0, 18,  6,  2,  2,  0,  0 },  // 6
        { 0,  7,  2,  1, 19,  6,  2,  2,  1,  0 },  // 7
        { 0,  8,  2,  2, 20,  6,  2,  2,  2,  0 },  // 8
        { 1,  0,  0,  3,  3,  0,  0,  0,  0,  1 },  // 9
        { 1,  1,  0,  4,  4,  0,  0,  0,  1,  1 },  // 10
        { 1,  2,  0,  5,  5,  0,  0,  0,  2,  1 },  // 11
        { 1,  3,  1,  3, 12,  3,  1,  1,  0,  1 },  // 12
        { 1,  4,  1,  4, 13,  3,  1,  1,  1,  1 },  // 13
        { 1,  5,  1,  5, 14,  3,  1,  1,  2,  1 },  // 14
        { 1,  6,  2,  3, 21,  6,  2,  2,  0,  1 },  // 15
        { 1,  7,  2,  4, 22,  6,  2,  2,  1,  1 },  // 16
        { 1,  8,  2,  5, 23,  6,  2,  2,  2,  1 },  // 17
        { 2,  0,  0,  6,  6,  0,  0,  0,  0,  2 },  // 18
        { 2,  1,  0,  7,  7,  0,  0,  0,  1,  2 },  // 19
        { 2,  2,  0,  8,  8,  0,  0,  0,  2,  2 },  // 20
        { 2,  3,  1,  6, 15,  3,  1,  1,  0,  2 },  // 21
        { 2,  4,  1,  7, 16,  3,  1,  1,  1,  2 },  // 22
        { 2,  5,  1,  8, 17,  3,  1,  1,  2,  2 },  // 23
        { 2,  6,  2,  6, 24,  6,  2,  2,  0,  2 },  // 24
        { 2,  7,  2,  7, 25,  6,  2,  2,  1,  2 },  // 25
        { 2,  8,  2,  8, 26,  6,  2,  2,  2,  2 },  // 26
        { 3,  0,  3,  0, 27, 27,  0,  0,  0,  0 },  // 27
        { 3,  1,  3,  1, 28, 27,  0,  0,  1,  0 },  // 28
        { 3,  2,  3,  2, 29, 27,  0,  0,  2,  0 },  // 29
        { 3,  3,  4,  0, 36, 30,  1,  1,  0,  0 },  // 30
        { 3,  4,  4,  1, 37, 30,  1,  1,  1,  0 },  // 31
        { 3,  5,  4,  2, 38, 30,  1,  1,  2,  0 },  // 32
        { 3,  6,  5,  0, 45, 33,  2,  2,  0,  0 },  // 33
        { 3,  7,  5,  1, 46, 33,  2,  2,  1,  0 },  // 34
        { 3,  8,  5,  2, 47, 33,  2,  2,  2,  0 },  // 35
        { 4,  0,  3,  3, 30, 27,  0,  0,  0,  1 },  // 36
        { 4,  1,  3,  4, 31, 27,  0,  0,  1,  1 },  // 37
        { 4,  2,  3,  5, 32, 27,  0,  0,  2,  1 },  // 38
        { 4,  3,  4,  3, 39, 30,  1,  1,  0,  1 },  // 39
        { 4,  4,  4,  4, 40, 30,  1,  1,  1,  1 },  // 40
        { 4,  5,  4,  5, 41, 30,  1,  1,  2,  1 },  // 41
        { 4,  6,  5,  3, 48, 33,  2,  2,  0,  1 },  // 42
        { 4,  7,  5,  4, 49, 33,  2,  2,  1,  1 },  // 43
        { 4,  8,  5,  5, 50, 33,  2,  2,  2,  1 },  // 44
        { 5,  0,  3,  6, 33, 27,  0,  0,  0,  2 },  // 45
        { 5,  1,  3,  7, 34, 27,  0,  0,  1,  2 },  // 46
        { 5,  2,  3,  8, 35, 27,  0,  0,  2,  2 },  // 47
        { 5,  3,  4,  6, 42, 30,  1,  1,  0,  2 },  // 48
        { 5,  4,  4,  7, 43, 30,  1,  1,  1,  2 },  // 49
        { 5,  5,  4,  8, 44, 30,  1,  1,  2,  2 },  // 50
        { 5,  6,  5,  6, 51, 33,  2,  2,  0,  2 },  // 51
        { 5,  7,  5,  7, 52, 33,  2,  2,  1,  2 },  // 52
        { 5,  8,  5,  8, 53, 33,  2,  2,  2,  2 },  // 53
        { 6,  0,  6,  0, 54, 54,  0,  0,  0,  0 },  // 54
        { 6,  1,  6,  1, 55, 54,  0,  0,  1,  0 },  // 55
        { 6,  2,  6,  2, 56, 54,  0,  0,  2,  0 },  // 56
        { 6,  3,  7,  0, 63, 57,  1,  1,  0,  0 },  // 57
        { 6,  4,  7,  1, 64, 57,  1,  1,  1,  0 },  // 58
        { 6,  5,  7,  2, 65, 57,  1,  1,  2,  0 },  // 59
        { 6,  6,  8,  0, 72, 60,  2,  2,  0,  0 },  // 60
        { 6,  7,  8,  1, 73, 60,  2,  2,  1,  0 },  // 61
        { 6,  8,  8,  2, 74, 60,  2,  2,  2,  0 },  // 62
        { 7,  0,  6,  3, 57, 54,  0,  0,  0,  1 },  // 63
        { 7,  1,  6,  4, 58, 54,  0,  0,  1,  1 },  // 64
        { 7,  2,  6,  5, 59, 54,  0,  0,  2,  1 },  // 65
        { 7,  3,  7,  3, 66, 57,  1,  1,  0,  1 },  // 66
        { 7,  4,  7,  4, 67, 57,  1,  1,  1,  1 },  // 67
        { 7,  5,  7,  5, 68, 57,  1,  1,  2,  1 },  // 68
        { 7,  6,  8,  3, 75, 60,  2,  2,  0,  1 },  // 69
        { 7,  7,  8,  4, 76, 60,  2,  2,  1,  1 },  // 70
        { 7,  8,  8,  5, 77, 60,  2,  2,  2,  1 },  // 71
        { 8,  0,  6,  6, 60, 54,  0,  0,  0,  2 },  // 72
        { 8,  1,  6,  7, 61, 54,  0,  0,  1,  2 },  // 73
        { 8,  2,  6,  8, 62, 54,  0,  0,  2,  2 },  // 74
        { 8,  3,  7,  6, 69, 57,  1,  1,  0,  2 },  // 75
        { 8,  4,  7,  7, 70, 57,  1,  1,  1,  2 },  // 76
        { 8,  5,  7,  8, 71, 57,  1,  1,  2,  2 },  // 77
        { 8,  6,  8,  6, 78, 60,  2,  2,  0,  2 },  // 78
        { 8,  7,  8,  7, 79, 60,  2,  2,  1,  2 },  // 79
        { 8,  8,  8,  8, 80, 60,  2,  2,  2,  2 }   // 80
    };

    const uint8_t boxes_info[81][10] = {
        { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },  // 0
        { 0,  1,  0,  1,  1,  0,  0,  0,  1,  0 },  // 1
        { 0,  2,  0,  2,  2,  0,  0,  0,  2,  0 },  // 2
        { 1,  0,  0,  3,  9,  0,  0,  0,  0,  1 },  // 3
        { 1,  1,  0,  4, 10,  0,  0,  0,  1,  1 },  // 4
        { 1,  2,  0,  5, 11,  0,  0,  0,  2,  1 },  // 5
        { 2,  0,  0,  6, 18,  0,  0,  0,  0,  2 },  // 6
        { 2,  1,  0,  7, 19,  0,  0,  0,  1,  2 },  // 7
        { 2,  2,  0,  8, 20,  0,  0,  0,  2,  2 },  // 8
        { 0,  3,  1,  0,  3,  3,  1,  1,  0,  0 },  // 9
        { 0,  4,  1,  1,  4,  3,  1,  1,  1,  0 },  // 10
        { 0,  5,  1,  2,  5,  3,  1,  1,  2,  0 },  // 11
        { 1,  3,  1,  3, 12,  3,  1,  1,  0,  1 },  // 12
        { 1,  4,  1,  4, 13,  3,  1,  1,  1,  1 },  // 13
        { 1,  5,  1,  5, 14,  3,  1,  1,  2,  1 },  // 14
        { 2,  3,  1,  6, 21,  3,  1,  1,  0,  2 },  // 15
        { 2,  4,  1,  7, 22,  3,  1,  1,  1,  2 },  // 16
        { 2,  5,  1,  8, 23,  3,  1,  1,  2,  2 },  // 17
        { 0,  6,  2,  0,  6,  6,  2,  2,  0,  0 },  // 18
        { 0,  7,  2,  1,  7,  6,  2,  2,  1,  0 },  // 19
        { 0,  8,  2,  2,  8,  6,  2,  2,  2,  0 },  // 20
        { 1,  6,  2,  3, 15,  6,  2,  2,  0,  1 },  // 21
        { 1,  7,  2,  4, 16,  6,  2,  2,  1,  1 },  // 22
        { 1,  8,  2,  5, 17,  6,  2,  2,  2,  1 },  // 23
        { 2,  6,  2,  6, 24,  6,  2,  2,  0,  2 },  // 24
        { 2,  7,  2,  7, 25,  6,  2,  2,  1,  2 },  // 25
        { 2,  8,  2,  8, 26,  6,  2,  2,  2,  2 },  // 26
        { 3,  0,  3,  0, 27, 27,  0,  0,  0,  0 },  // 27
        { 3,  1,  3,  1, 28, 27,  0,  0,  1,  0 },  // 28
        { 3,  2,  3,  2, 29, 27,  0,  0,  2,  0 },  // 29
        { 4,  0,  3,  3, 36, 27,  0,  0,  0,  1 },  // 30
        { 4,  1,  3,  4, 37, 27,  0,  0,  1,  1 },  // 31
        { 4,  2,  3,  5, 38, 27,  0,  0,  2,  1 },  // 32
        { 5,  0,  3,  6, 45, 27,  0,  0,  0,  2 },  // 33
        { 5,  1,  3,  7, 46, 27,  0,  0,  1,  2 },  // 34
        { 5,  2,  3,  8, 47, 27,  0,  0,  2,  2 },  // 35
        { 3,  3,  4,  0, 30, 30,  1,  1,  0,  0 },  // 36
        { 3,  4,  4,  1, 31, 30,  1,  1,  1,  0 },  // 37
        { 3,  5,  4,  2, 32, 30,  1,  1,  2,  0 },  // 38
        { 4,  3,  4,  3, 39, 30,  1,  1,  0,  1 },  // 39
        { 4,  4,  4,  4, 40, 30,  1,  1,  1,  1 },  // 40
        { 4,  5,  4,  5, 41, 30,  1,  1,  2,  1 },  // 41
        { 5,  3,  4,  6, 48, 30,  1,  1,  0,  2 },  // 42
        { 5,  4,  4,  7, 49, 30,  1,  1,  1,  2 },  // 43
        { 5,  5,  4,  8, 50, 30,  1,  1,  2,  2 },  // 44
        { 3,  6,  5,  0, 33, 33,  2,  2,  0,  0 },  // 45
        { 3,  7,  5,  1, 34, 33,  2,  2,  1,  0 },  // 46
        { 3,  8,  5,  2, 35, 33,  2,  2,  2,  0 },  // 47
        { 4,  6,  5,  3, 42, 33,  2,  2,  0,  1 },  // 48
        { 4,  7,  5,  4, 43, 33,  2,  2,  1,  1 },  // 49
        { 4,  8,  5,  5, 44, 33,  2,  2,  2,  1 },  // 50
        { 5,  6,  5,  6, 51, 33,  2,  2,  0,  2 },  // 51
        { 5,  7,  5,  7, 52, 33,  2,  2,  1,  2 },  // 52
        { 5,  8,  5,  8, 53, 33,  2,  2,  2,  2 },  // 53
        { 6,  0,  6,  0, 54, 54,  0,  0,  0,  0 },  // 54
        { 6,  1,  6,  1, 55, 54,  0,  0,  1,  0 },  // 55
        { 6,  2,  6,  2, 56, 54,  0,  0,  2,  0 },  // 56
        { 7,  0,  6,  3, 63, 54,  0,  0,  0,  1 },  // 57
        { 7,  1,  6,  4, 64, 54,  0,  0,  1,  1 },  // 58
        { 7,  2,  6,  5, 65, 54,  0,  0,  2,  1 },  // 59
        { 8,  0,  6,  6, 72, 54,  0,  0,  0,  2 },  // 60
        { 8,  1,  6,  7, 73, 54,  0,  0,  1,  2 },  // 61
        { 8,  2,  6,  8, 74, 54,  0,  0,  2,  2 },  // 62
        { 6,  3,  7,  0, 57, 57,  1,  1,  0,  0 },  // 63
        { 6,  4,  7,  1, 58, 57,  1,  1,  1,  0 },  // 64
        { 6,  5,  7,  2, 59, 57,  1,  1,  2,  0 },  // 65
        { 7,  3,  7,  3, 66, 57,  1,  1,  0,  1 },  // 66
        { 7,  4,  7,  4, 67, 57,  1,  1,  1,  1 },  // 67
        { 7,  5,  7,  5, 68, 57,  1,  1,  2,  1 },  // 68
        { 8,  3,  7,  6, 75, 57,  1,  1,  0,  2 },  // 69
        { 8,  4,  7,  7, 76, 57,  1,  1,  1,  2 },  // 70
        { 8,  5,  7,  8, 77, 57,  1,  1,  2,  2 },  // 71
        { 6,  6,  8,  0, 60, 60,  2,  2,  0,  0 },  // 72
        { 6,  7,  8,  1, 61, 60,  2,  2,  1,  0 },  // 73
        { 6,  8,  8,  2, 62, 60,  2,  2,  2,  0 },  // 74
        { 7,  6,  8,  3, 69, 60,  2,  2,  0,  1 },  // 75
        { 7,  7,  8,  4, 70, 60,  2,  2,  1,  1 },  // 76
        { 7,  8,  8,  5, 71, 60,  2,  2,  2,  1 },  // 77
        { 8,  6,  8,  6, 78, 60,  2,  2,  0,  2 },  // 78
        { 8,  7,  8,  7, 79, 60,  2,  2,  1,  2 },  // 79
        { 8,  8,  8,  8, 80, 60,  2,  2,  2,  2 }   // 80
    };

    const uint8_t neighbor_cells[81][20] = {
        { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 18, 19, 20, 27, 36, 45, 54, 63, 72 },  // 0
        { 0,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 18, 19, 20, 28, 37, 46, 55, 64, 73 },  // 1
        { 0,  1,  3,  4,  5,  6,  7,  8,  9, 10, 11, 18, 19, 20, 29, 38, 47, 56, 65, 74 },  // 2
        { 0,  1,  2,  4,  5,  6,  7,  8, 12, 12, 13, 14, 21, 21, 30, 39, 48, 57, 66, 75 },  // 3
        { 0,  1,  2,  3,  5,  6,  7,  8, 12, 13, 13, 14, 21, 22, 31, 40, 49, 58, 67, 76 },  // 4
        { 0,  1,  2,  3,  4,  6,  7,  8, 12, 13, 14, 14, 21, 23, 32, 41, 50, 59, 68, 77 },  // 5
        { 0,  1,  2,  3,  4,  5,  7,  8, 15, 15, 16, 17, 24, 24, 33, 42, 51, 60, 69, 78 },  // 6
        { 0,  1,  2,  3,  4,  5,  6,  8, 15, 16, 16, 17, 24, 25, 34, 43, 52, 61, 70, 79 },  // 7
        { 0,  1,  2,  3,  4,  5,  6,  7, 15, 16, 17, 17, 24, 26, 35, 44, 53, 62, 71, 80 },  // 8
        { 0,  1,  2, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 27, 36, 45, 54, 63, 72 },  // 9
        { 0,  1,  2,  9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 28, 37, 46, 55, 64, 73 },  // 10
        { 0,  1,  2,  9, 10, 12, 13, 14, 15, 16, 17, 18, 19, 20, 29, 38, 47, 56, 65, 74 },  // 11
        { 3,  3,  4,  5,  9, 10, 11, 13, 14, 15, 16, 17, 21, 21, 30, 39, 48, 57, 66, 75 },  // 12
        { 3,  4,  4,  5,  9, 10, 11, 12, 14, 15, 16, 17, 21, 22, 31, 40, 49, 58, 67, 76 },  // 13
        { 3,  4,  5,  5,  9, 10, 11, 12, 13, 15, 16, 17, 21, 23, 32, 41, 50, 59, 68, 77 },  // 14
        { 6,  6,  7,  8,  9, 10, 11, 12, 13, 14, 16, 17, 24, 24, 33, 42, 51, 60, 69, 78 },  // 15
        { 6,  7,  7,  8,  9, 10, 11, 12, 13, 14, 15, 17, 24, 25, 34, 43, 52, 61, 70, 79 },  // 16
        { 6,  7,  8,  8,  9, 10, 11, 12, 13, 14, 15, 16, 24, 26, 35, 44, 53, 62, 71, 80 },  // 17
        { 0,  1,  2,  9, 10, 11, 19, 20, 21, 22, 23, 24, 25, 26, 27, 36, 45, 54, 63, 72 },  // 18
        { 0,  1,  2,  9, 10, 11, 18, 20, 21, 22, 23, 24, 25, 26, 28, 37, 46, 55, 64, 73 },  // 19
        { 0,  1,  2,  9, 10, 11, 18, 19, 21, 22, 23, 24, 25, 26, 29, 38, 47, 56, 65, 74 },  // 20
        { 3,  3,  4,  5, 12, 12, 18, 19, 20, 22, 23, 24, 25, 26, 30, 39, 48, 57, 66, 75 },  // 21
        { 3,  4,  4,  5, 12, 13, 18, 19, 20, 21, 23, 24, 25, 26, 31, 40, 49, 58, 67, 76 },  // 22
        { 3,  4,  5,  5, 12, 14, 18, 19, 20, 21, 22, 24, 25, 26, 32, 41, 50, 59, 68, 77 },  // 23
        { 6,  6,  7,  8, 15, 15, 18, 19, 20, 21, 22, 23, 25, 26, 33, 42, 51, 60, 69, 78 },  // 24
        { 6,  7,  7,  8, 15, 16, 18, 19, 20, 21, 22, 23, 24, 26, 34, 43, 52, 61, 70, 79 },  // 25
        { 6,  7,  8,  8, 15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 35, 44, 53, 62, 71, 80 },  // 26
        { 0,  9, 18, 28, 28, 29, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 45, 54, 63, 72 },  // 27
        { 1, 10, 19, 27, 27, 29, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 46, 55, 64, 73 },  // 28
        { 2, 11, 20, 27, 27, 28, 28, 30, 31, 32, 33, 34, 35, 36, 37, 38, 47, 56, 65, 74 },  // 29
        { 3, 12, 21, 27, 28, 29, 31, 31, 32, 32, 33, 34, 35, 39, 39, 40, 48, 57, 66, 75 },  // 30
        { 4, 13, 22, 27, 28, 29, 30, 30, 32, 32, 33, 34, 35, 39, 40, 40, 49, 58, 67, 76 },  // 31
        { 5, 14, 23, 27, 28, 29, 30, 30, 31, 31, 33, 34, 35, 39, 40, 41, 50, 59, 68, 77 },  // 32
        { 6, 15, 24, 27, 28, 29, 30, 31, 32, 34, 34, 35, 35, 42, 42, 43, 51, 60, 69, 78 },  // 33
        { 7, 16, 25, 27, 28, 29, 30, 31, 32, 33, 33, 35, 35, 42, 43, 43, 52, 61, 70, 79 },  // 34
        { 8, 17, 26, 27, 28, 29, 30, 31, 32, 33, 33, 34, 34, 42, 43, 44, 53, 62, 71, 80 },  // 35
        { 0,  9, 18, 27, 28, 29, 37, 37, 38, 38, 39, 40, 41, 42, 43, 44, 45, 54, 63, 72 },  // 36
        { 1, 10, 19, 27, 28, 29, 36, 36, 38, 38, 39, 40, 41, 42, 43, 44, 46, 55, 64, 73 },  // 37
        { 2, 11, 20, 27, 28, 29, 36, 36, 37, 37, 39, 40, 41, 42, 43, 44, 47, 56, 65, 74 },  // 38
        { 3, 12, 21, 30, 30, 31, 32, 36, 37, 38, 40, 40, 41, 42, 43, 44, 48, 57, 66, 75 },  // 39
        { 4, 13, 22, 30, 31, 31, 32, 36, 37, 38, 39, 39, 41, 42, 43, 44, 49, 58, 67, 76 },  // 40
        { 5, 14, 23, 30, 31, 32, 32, 36, 37, 38, 39, 39, 40, 42, 43, 44, 50, 59, 68, 77 },  // 41
        { 6, 15, 24, 33, 33, 34, 35, 36, 37, 38, 39, 40, 41, 43, 43, 44, 51, 60, 69, 78 },  // 42
        { 7, 16, 25, 33, 34, 34, 35, 36, 37, 38, 39, 40, 41, 42, 42, 44, 52, 61, 70, 79 },  // 43
        { 8, 17, 26, 33, 34, 35, 35, 36, 37, 38, 39, 40, 41, 42, 42, 43, 53, 62, 71, 80 },  // 44
        { 0,  9, 18, 27, 28, 29, 36, 37, 38, 46, 47, 48, 49, 50, 51, 52, 53, 54, 63, 72 },  // 45
        { 1, 10, 19, 27, 28, 29, 36, 37, 38, 45, 47, 48, 49, 50, 51, 52, 53, 55, 64, 73 },  // 46
        { 2, 11, 20, 27, 28, 29, 36, 37, 38, 45, 46, 48, 49, 50, 51, 52, 53, 56, 65, 74 },  // 47
        { 3, 12, 21, 30, 30, 31, 32, 39, 39, 45, 46, 47, 49, 50, 51, 52, 53, 57, 66, 75 },  // 48
        { 4, 13, 22, 30, 31, 31, 32, 39, 40, 45, 46, 47, 48, 50, 51, 52, 53, 58, 67, 76 },  // 49
        { 5, 14, 23, 30, 31, 32, 32, 39, 41, 45, 46, 47, 48, 49, 51, 52, 53, 59, 68, 77 },  // 50
        { 6, 15, 24, 33, 33, 34, 35, 42, 42, 45, 46, 47, 48, 49, 50, 52, 53, 60, 69, 78 },  // 51
        { 7, 16, 25, 33, 34, 34, 35, 42, 43, 45, 46, 47, 48, 49, 50, 51, 53, 61, 70, 79 },  // 52
        { 8, 17, 26, 33, 34, 35, 35, 42, 44, 45, 46, 47, 48, 49, 50, 51, 52, 62, 71, 80 },  // 53
        { 0,  9, 18, 27, 36, 45, 55, 55, 56, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 72 },  // 54
        { 1, 10, 19, 28, 37, 46, 54, 54, 56, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 73 },  // 55
        { 2, 11, 20, 29, 38, 47, 54, 54, 55, 55, 57, 58, 59, 60, 61, 62, 63, 64, 65, 74 },  // 56
        { 3, 12, 21, 30, 39, 48, 54, 55, 56, 58, 58, 59, 59, 60, 61, 62, 66, 66, 67, 75 },  // 57
        { 4, 13, 22, 31, 40, 49, 54, 55, 56, 57, 57, 59, 59, 60, 61, 62, 66, 67, 67, 76 },  // 58
        { 5, 14, 23, 32, 41, 50, 54, 55, 56, 57, 57, 58, 58, 60, 61, 62, 66, 67, 68, 77 },  // 59
        { 6, 15, 24, 33, 42, 51, 54, 55, 56, 57, 58, 59, 61, 61, 62, 62, 69, 69, 70, 78 },  // 60
        { 7, 16, 25, 34, 43, 52, 54, 55, 56, 57, 58, 59, 60, 60, 62, 62, 69, 70, 70, 79 },  // 61
        { 8, 17, 26, 35, 44, 53, 54, 55, 56, 57, 58, 59, 60, 60, 61, 61, 69, 70, 71, 80 },  // 62
        { 0,  9, 18, 27, 36, 45, 54, 55, 56, 64, 64, 65, 65, 66, 67, 68, 69, 70, 71, 72 },  // 63
        { 1, 10, 19, 28, 37, 46, 54, 55, 56, 63, 63, 65, 65, 66, 67, 68, 69, 70, 71, 73 },  // 64
        { 2, 11, 20, 29, 38, 47, 54, 55, 56, 63, 63, 64, 64, 66, 67, 68, 69, 70, 71, 74 },  // 65
        { 3, 12, 21, 30, 39, 48, 57, 57, 58, 59, 63, 64, 65, 67, 67, 68, 69, 70, 71, 75 },  // 66
        { 4, 13, 22, 31, 40, 49, 57, 58, 58, 59, 63, 64, 65, 66, 66, 68, 69, 70, 71, 76 },  // 67
        { 5, 14, 23, 32, 41, 50, 57, 58, 59, 59, 63, 64, 65, 66, 66, 67, 69, 70, 71, 77 },  // 68
        { 6, 15, 24, 33, 42, 51, 60, 60, 61, 62, 63, 64, 65, 66, 67, 68, 70, 70, 71, 78 },  // 69
        { 7, 16, 25, 34, 43, 52, 60, 61, 61, 62, 63, 64, 65, 66, 67, 68, 69, 69, 71, 79 },  // 70
        { 8, 17, 26, 35, 44, 53, 60, 61, 62, 62, 63, 64, 65, 66, 67, 68, 69, 69, 70, 80 },  // 71
        { 0,  9, 18, 27, 36, 45, 54, 55, 56, 63, 64, 65, 73, 74, 75, 76, 77, 78, 79, 80 },  // 72
        { 1, 10, 19, 28, 37, 46, 54, 55, 56, 63, 64, 65, 72, 74, 75, 76, 77, 78, 79, 80 },  // 73
        { 2, 11, 20, 29, 38, 47, 54, 55, 56, 63, 64, 65, 72, 73, 75, 76, 77, 78, 79, 80 },  // 74
        { 3, 12, 21, 30, 39, 48, 57, 57, 58, 59, 66, 66, 72, 73, 74, 76, 77, 78, 79, 80 },  // 75
        { 4, 13, 22, 31, 40, 49, 57, 58, 58, 59, 66, 67, 72, 73, 74, 75, 77, 78, 79, 80 },  // 76
        { 5, 14, 23, 32, 41, 50, 57, 58, 59, 59, 66, 68, 72, 73, 74, 75, 76, 78, 79, 80 },  // 77
        { 6, 15, 24, 33, 42, 51, 60, 60, 61, 62, 69, 69, 72, 73, 74, 75, 76, 77, 79, 80 },  // 78
        { 7, 16, 25, 34, 43, 52, 60, 61, 61, 62, 69, 70, 72, 73, 74, 75, 76, 77, 78, 80 },  // 79
        { 8, 17, 26, 35, 44, 53, 60, 61, 62, 62, 69, 71, 72, 73, 74, 75, 76, 77, 78, 79 }   // 80
    };

    sTables() noexcept {
        //
    }
};

static const sTables s_tables {};

struct Sudoku {
    static const size_t kAlignment = sizeof(size_t);

    static const size_t kBoxCellsX = 3;
    static const size_t kBoxCellsY = 3;
    static const size_t kBoxCountX = 3;
    static const size_t kBoxCountY = 3;
    static const size_t kMinNumber = 1;
    static const size_t kMaxNumber = 9;

    static const size_t kCols = kBoxCellsX * kBoxCountX;
    static const size_t kRows = kBoxCellsY * kBoxCountY;
    static const size_t kBoxes = kBoxCountX * kBoxCountY;
    static const size_t kNumbers = (kMaxNumber - kMinNumber) + 1;

    static const size_t kBoxSize = kBoxCellsX * kBoxCellsY;
    static const size_t kBoardSize = kRows * kCols;
    static const size_t kTotalSize = kRows * kCols * kNumbers;
    static const size_t kTotalSize2 = kBoxes * kBoxSize * kNumbers;

    static const size_t kNeighbors = (kCols - 1) + (kRows - 1) +
                                     (kBoxSize - (kBoxCellsX - 1) - (kBoxCellsY - 1) - 1);

    static const size_t kRows16 = AlignedTo<kRows, 16>::value;
    static const size_t kCols16 = AlignedTo<kCols, 16>::value;
    static const size_t kNumbers10 = AlignedTo<kNumbers, 2>::value;
    static const size_t kNumbers16 = AlignedTo<kNumbers, 16>::value;
    static const size_t kBoxes16 = AlignedTo<kBoxes, 16>::value;
    static const size_t kBoxSize16 = AlignedTo<kBoxSize, 16>::value;
    static const size_t kBoardSize16 = kBoxes16 * kBoxSize16;

    static const size_t kMaxEffectBox = (kBoxCountX - 1) + (kBoxCountY -1) + 1;
    static const size_t kMaxEffectLength = kMaxEffectBox * kBoxSize;

    static const size_t kTotalCellLiterals = kRows * kCols;
    static const size_t kTotalRowLiterals = kRows * kNumbers;
    static const size_t kTotalColLiterals = kCols * kNumbers;
    static const size_t kTotalBoxLiterals = kBoxes * kNumbers;

    static const size_t kTotalLiterals =
        kTotalCellLiterals + kTotalRowLiterals + kTotalColLiterals + kTotalBoxLiterals;

    static const size_t kLiteralFirst     = 0;
    static const size_t kCellLiteralFirst = kLiteralFirst;
    static const size_t kRowLiteralFirst  = kCellLiteralFirst + kTotalCellLiterals;
    static const size_t kColLiteralFirst  = kRowLiteralFirst + kTotalRowLiterals;
    static const size_t kBoxLiteralFirst  = kColLiteralFirst + kTotalColLiterals;
    static const size_t kLiteralLast      = kBoxLiteralFirst + kTotalBoxLiterals;

    static const size_t kAllRowBits = (size_t(1) << kRows) - 1;
    static const size_t kAllColBits = (size_t(1) << kCols) - 1;
    static const size_t kAllBoxBits = (size_t(1) << kBoxes) - 1;
    static const size_t kAllBoxCellBits = (size_t(1) << kBoxSize) - 1;
    static const size_t kAllNumberBits = (size_t(1) << kNumbers) - 1;

    static const bool kAllDimIsSame = ((kNumbers == kBoxSize) && (kRows == kCols) && (kNumbers == kRows));

    static const size_t kMinInitCandidates = 17;

    typedef Board board_type;

#pragma pack(push, 1)

    static const size_t kNeighborsAlignBytes    = ((kNeighbors * sizeof(uint8_t) + kAlignment - 1) / kAlignment) * kAlignment;
    static const size_t kNeighborsReserveBytes1 = kNeighborsAlignBytes - kNeighbors * sizeof(uint8_t);
    static const size_t kNeighborsReserveBytes  = (kNeighborsReserveBytes1 != 0) ? kNeighborsReserveBytes1 : kAlignment;

    // Aligned to sizeof(size_t) for cache friendly
    struct NeighborCells {
        uint8_t cells[kNeighbors];
        uint8_t reserve[kNeighborsReserveBytes];
    };

    struct CellInfo {
        uint8_t row, col;
        uint8_t box, cell;
        uint8_t box_pos;
        uint8_t box_base;
        uint8_t box_x, box_y;
        uint8_t cell_x, cell_y;
        // Reserve for cache friendly
        uint8_t reserve[6];
    };

    struct BoxesInfo {
        uint8_t row, col;
        uint8_t box, cell;
        uint8_t pos;
        uint8_t box_base;
        uint8_t box_x, box_y;
        uint8_t cell_x, cell_y;
        // Reserve for cache friendly
        uint8_t reserve[6];
    };

#pragma pack(pop)

    typedef SmallBitSet<kBoardSize>               BitMask;
    typedef SmallBitSet2D<kBoardSize, kBoardSize> BitMaskTable;

    static bool is_inited;

    static CellInfo *       cell_info;
    static CellInfo *       cell_info16;
    static BoxesInfo *      boxes_info;
    static BoxesInfo *      boxes_info16;
    static NeighborCells *  neighbor_cells;
    static NeighborCells *  ordered_neighbor_cells;
    static BitMaskTable     neighbors_mask_tbl;

    Sudoku() {
        Sudoku::initialize();
    }

    ~Sudoku() {
        Sudoku::finalize();
    }

    static void initialize() {
        //
        // See: https://stackoverflow.com/questions/40579342/is-there-any-compiler-barrier-which-is-equal-to-asm-memory-in-c11
        //
        std::atomic_signal_fence(std::memory_order_release);        // _compile_barrier()
        if (!is_inited) {
            printf("Sudoku::initialize()\n");
            neighbors_mask_tbl.reset();
            make_cell_info();
            make_boxes_info();
            make_neighbor_cells();
            is_inited = true;
        }
        std::atomic_signal_fence(std::memory_order_release);        // _compile_barrier()
    }

    static void finalize() {
        std::atomic_signal_fence(std::memory_order_release);        // _compile_barrier()
        if (is_inited) {
            printf("Sudoku::finalize()\n");
            if (cell_info) {
                delete[] cell_info;
                cell_info = nullptr;
            }
            if (cell_info16) {
                delete[] cell_info16;
                cell_info16 = nullptr;
            }
            if (boxes_info) {
                delete[] boxes_info;
                boxes_info = nullptr;
            }
            if (boxes_info16) {
                delete[] boxes_info16;
                boxes_info16 = nullptr;
            }
            if (neighbor_cells) {
                delete[] neighbor_cells;
                neighbor_cells = nullptr;
            }
            if (ordered_neighbor_cells) {
                delete[] ordered_neighbor_cells;
                ordered_neighbor_cells = nullptr;
            }
            is_inited = false;
        }
        std::atomic_signal_fence(std::memory_order_release);        // _compile_barrier()
    }

    static void make_cell_info() {
        if (cell_info == nullptr) {
            cell_info = new CellInfo[kBoardSize];
            cell_info16 = new CellInfo[kRows * kCols16];

            std::memset(cell_info, 0, sizeof(CellInfo) * kBoardSize);
            std::memset(cell_info16, 0, sizeof(CellInfo) * kRows * kCols16);

            size_t pos = 0;
            for (size_t row = 0; row < kRows; row++) {
                for (size_t col = 0; col < kCols; col++) {
                    CellInfo * cellInfo = &cell_info[pos];
                    CellInfo * cellInfo16 = &cell_info16[row * kCols16 + col];

                    size_t box_x = col / kBoxCellsX;
                    size_t box_y = row / kBoxCellsY;
                    size_t box = box_y * kBoxCountX + box_x;
                    size_t box_base = (box_y * kBoxCellsY) * kCols + box_x * kBoxCellsX;
                    size_t cell_x = col % kBoxCellsX;
                    size_t cell_y = row % kBoxCellsY;
                    size_t cell = cell_y * kBoxCellsX + cell_x;
                    size_t box_pos = box * kBoxSize + cell;

                    cellInfo->row = (uint8_t)row;
                    cellInfo->col = (uint8_t)col;
                    cellInfo->box = (uint8_t)box;
                    cellInfo->cell = (uint8_t)cell;
                    cellInfo->box_pos = (uint8_t)box_pos;
                    cellInfo->box_base = (uint8_t)box_base;
                    cellInfo->box_x = (uint8_t)box_x;
                    cellInfo->box_y = (uint8_t)box_x;
                    cellInfo->cell_x = (uint8_t)cell_x;
                    cellInfo->cell_y = (uint8_t)cell_y;

                    cellInfo16->row = (uint8_t)row;
                    cellInfo16->col = (uint8_t)col;
                    cellInfo16->box = (uint8_t)box;
                    cellInfo16->cell = (uint8_t)cell;
                    cellInfo16->box_pos = (uint8_t)box_pos;
                    cellInfo16->box_base = (uint8_t)box_base;
                    cellInfo16->box_x = (uint8_t)box_x;
                    cellInfo16->box_y = (uint8_t)box_x;
                    cellInfo16->cell_x = (uint8_t)cell_x;
                    cellInfo16->cell_y = (uint8_t)cell_y;

                    pos++;
                }
            }

            // print_cell_info();
        }
    }

    static void print_cell_info() {
        printf("    const uint8_t cell_info[%d][10] = {\n", (int)kBoardSize);
        for (size_t pos = 0; pos < kBoardSize; pos++) {
            printf("        {");
            const CellInfo & cellInfo = cell_info[pos];
            printf("%2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d",
                   (int)cellInfo.row, (int)cellInfo.col,
                   (int)cellInfo.box, (int)cellInfo.cell,
                   (int)cellInfo.box_pos, (int)cellInfo.box_base,
                   (int)cellInfo.box_x, (int)cellInfo.box_y,
                   (int)cellInfo.cell_x, (int)cellInfo.cell_y);
            if (pos < (kBoardSize - 1))
                printf(" },  // %d\n", (int)pos);
            else
                printf(" }   // %d\n", (int)pos);
        }
        printf("    };\n\n");
    }

    static void make_boxes_info() {
        if (boxes_info == nullptr) {
            boxes_info = new BoxesInfo[kBoxes * kBoxSize];
            boxes_info16 = new BoxesInfo[kBoxes * kBoxSize16];

            std::memset(boxes_info, 0, sizeof(BoxesInfo) * kBoxes * kBoxSize);
            std::memset(boxes_info16, 0, sizeof(BoxesInfo) * kBoxes * kBoxSize16);

            size_t index = 0;
            for (size_t box = 0; box < kBoxes; box++) {
                for (size_t cell = 0; cell < kBoxSize; cell++) {
                    BoxesInfo * boxesInfo = &boxes_info[index];
                    BoxesInfo * boxesInfo16 = &boxes_info16[box * kBoxSize16 + cell];

                    size_t row = (box / kBoxCountX) * kBoxCellsY + (cell / kBoxCellsX);
                    size_t col = (box % kBoxCountX) * kBoxCellsX + (cell % kBoxCellsX);
                    size_t pos = row * kCols + col;
                    size_t box_x = box % kBoxCountX;
                    size_t box_y = box / kBoxCountX;
                    size_t box_base = (box_y * kBoxCellsY) * kCols + box_x * kBoxCellsX;
                    size_t cell_x = col % kBoxCellsX;
                    size_t cell_y = row % kBoxCellsY;

                    boxesInfo->row = (uint8_t)row;
                    boxesInfo->col = (uint8_t)col;
                    boxesInfo->box = (uint8_t)box;
                    boxesInfo->cell = (uint8_t)cell;
                    boxesInfo->pos = (uint8_t)pos;
                    boxesInfo->box_base = (uint8_t)box_base;
                    boxesInfo->box_x = (uint8_t)box_x;
                    boxesInfo->box_y = (uint8_t)box_x;
                    boxesInfo->cell_x = (uint8_t)cell_x;
                    boxesInfo->cell_y = (uint8_t)cell_y;

                    boxesInfo16->row = (uint8_t)row;
                    boxesInfo16->col = (uint8_t)col;
                    boxesInfo16->box = (uint8_t)box;
                    boxesInfo16->cell = (uint8_t)cell;
                    boxesInfo16->pos = (uint8_t)pos;
                    boxesInfo16->box_base = (uint8_t)box_base;
                    boxesInfo16->box_x = (uint8_t)box_x;
                    boxesInfo16->box_y = (uint8_t)box_x;
                    boxesInfo16->cell_x = (uint8_t)cell_x;
                    boxesInfo16->cell_y = (uint8_t)cell_y;

                    index++;
                }
            }

            // print_boxes_info();
        }
    }

    static void print_boxes_info() {
        printf("    const uint8_t boxes_info[%d][10] = {\n", (int)(kBoxes * kBoxSize));
        for (size_t index = 0; index < kBoardSize; index++) {
            printf("        {");
            const BoxesInfo & boxesInfo = boxes_info[index];
            printf("%2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d",
                   (int)boxesInfo.row, (int)boxesInfo.col,
                   (int)boxesInfo.box, (int)boxesInfo.cell,
                   (int)boxesInfo.pos, (int)boxesInfo.box_base,
                   (int)boxesInfo.box_x, (int)boxesInfo.box_y,
                   (int)boxesInfo.cell_x, (int)boxesInfo.cell_y);
            if (index < (kBoxes * kBoxSize - 1))
                printf(" },  // %d\n", (int)index);
            else
                printf(" }   // %d\n", (int)index);
        }
        printf("    };\n\n");
    }

    static size_t get_neighbor_cells_list(size_t row, size_t col,
                                          NeighborCells * list) {
        assert(list != nullptr);
        size_t index = 0;
        size_t pos_y = row * kCols;
        for (size_t x = 0; x < kCols; x++) {
            if (x != col) {
                list->cells[index++] = (uint8_t)(pos_y + x);
            }
        }

        size_t pos_x = col;
        for (size_t y = 0; y < kRows; y++) {
            if (y != row) {
                list->cells[index++] = (uint8_t)(y * kCols + pos_x);
            }
        }

        size_t box_x = col / kBoxCellsX;
        size_t box_y = row / kBoxCellsY;
        size_t box_base = (box_y * kBoxCellsY) * kCols + box_x * kBoxCellsX;
        size_t pos = pos_y + pos_x;
        size_t cell_x = col % kBoxCellsX;
        size_t cell_y = row % kBoxCellsY;
        size_t cell = box_base;
        for (size_t y = 0; y < kBoxCellsY; y++) {
            if (y == cell_y) {
                cell += kCols;
            }
            else {
                for (size_t x = 0; x < kBoxCellsX; x++) {
                    if (x != cell_x) {
                        assert(cell != pos);
                        list->cells[index++] = (uint8_t)(cell);
                    }
                    cell++;
                }
                cell += (kCols - kBoxCellsX);
            }
        }

        assert(index == kNeighbors);
        return index;
    }

    static void make_effect_mask(size_t pos) {
        NeighborCells * list = &neighbor_cells[pos];
        SmallBitSet<kBoardSize> masks;
        for (size_t i = 0; i < kNeighbors; i++) {
            size_t cell = list->cells[i];
            masks.set(cell);
        }
        neighbors_mask_tbl[pos] = masks;
    }

    static void make_neighbor_cells() {
        if (neighbor_cells == nullptr) {
            neighbor_cells = new NeighborCells[kBoardSize];
            ordered_neighbor_cells = new NeighborCells[kBoardSize];

            size_t pos = 0;
            for (size_t row = 0; row < kRows; row++) {
                for (size_t col = 0; col < kCols; col++) {
                    NeighborCells * list = &neighbor_cells[pos];
                    size_t neighbors = get_neighbor_cells_list(row, col, list);
                    assert(neighbors == kNeighbors);
                    NeighborCells * ordered_list = &ordered_neighbor_cells[pos];
                    for (size_t cell = 0; cell < kNeighbors; cell++) {
                        ordered_list->cells[cell] = list->cells[cell];
                    }
                    // Sort the cells for cache friendly
                    std::sort(&neighbor_cells[pos].cells[0], &neighbor_cells[pos].cells[kNeighbors]);
                    make_effect_mask(pos);
                    pos++;
                }
            }

            // print_neighbor_cells();
            // print_ordered_neighbor_cells();
        }
    }

    static void print_neighbor_cells() {
        printf("    const uint8_t neighbor_cells[%d][%d] = {\n", (int)kBoardSize, (int)kNeighbors);
        for (size_t pos = 0; pos < kBoardSize; pos++) {
            printf("        { ");
            for (size_t cell = 0; cell < kNeighbors; cell++) {
                if (cell < kNeighbors - 1)
                    printf("%2u, ", (uint32_t)neighbor_cells[pos].cells[cell]);
                else
                    printf("%2u ", (uint32_t)neighbor_cells[pos].cells[cell]);
            }
            if (pos < (kBoardSize - 1))
                printf("},  // %d\n", (int)pos);
            else
                printf("}   // %d\n", (int)pos);
        }
        printf("    };\n\n");
    }

    static void print_ordered_neighbor_cells() {
        printf("    const uint8_t ordered_neighbor_cells[%d][%d] = {\n", (int)kBoardSize, (int)kNeighbors);
        for (size_t pos = 0; pos < kBoardSize; pos++) {
            printf("        { ");
            for (size_t cell = 0; cell < kNeighbors; cell++) {
                if (cell < kNeighbors - 1)
                    printf("%2u, ", (uint32_t)ordered_neighbor_cells[pos].cells[cell]);
                else
                    printf("%2u ", (uint32_t)ordered_neighbor_cells[pos].cells[cell]);
            }
            if (pos < (kBoardSize - 1))
                printf("},  // %d\n", (int)pos);
            else
                printf("}   // %d\n", (int)pos);
        }
        printf("    };\n\n");
    }

    static void clear_board(Board & board) {
        size_t pos = 0;
        for (size_t row = 0; row < kRows; row++) {
            for (size_t col = 0; col < kCols; col++) {
                board.cells[pos++] = '.';
            }
        }
    }

    static void display_board(Board & board,
                              bool is_input = false,
                              int idx = -1) {
        int filled = 0;
        size_t pos = 0;
        for (size_t row = 0; row < kRows; row++) {
            for (size_t col = 0; col < kCols; col++) {
                char val = board.cells[pos++];
                if ((val != '.') && (val != ' ') && (val != '0') && (val != '-')) {
                    filled++;
                }
            }
        }

        if (is_input) {
            printf("The input is: (filled = %d)\n", filled);
        }
        else {
            if (idx == -1)
                printf("The answer is:\n");
            else
                printf("The answer # %d is:\n", idx + 1);
        }
        printf("\n");
        // printf("  ------- ------- -------\n");
        printf(" ");
        for (size_t countX = 0; countX < kBoxCountX; countX++) {
            printf(" -------");
        }
        printf("\n");
        pos = 0;
        for (size_t row = 0; row < kRows; row++) {
            printf(" | ");
            for (size_t col = 0; col < kCols; col++) {
                unsigned char val = (unsigned char)board.cells[pos++];
                //if (val == ' ' || val == '0' || val == '-')
                if (val == ' ' || val == '-')
                    printf(". ");
                else if (val == '\0')
                    printf("* ");
                else if (val < ' ')
                    printf("? ");
                else
                    printf("%c ", val);
                if ((col % kBoxCellsX) == (kBoxCellsX - 1))
                    printf("| ");
            }
            printf("\n");
            if ((row % kBoxCellsY) == (kBoxCellsY - 1)) {
                // printf("  ------- ------- -------\n");
                printf(" ");
                for (size_t countX = 0; countX < kBoxCountX; countX++) {
                    printf(" -------");
                }
                printf("\n");
            }
        }
        printf("\n");
    }

    static void display_board(std::vector<Board> & board_list) {
        printf("Total answers: %d\n\n", (int)board_list.size());
        int i = 0;
        for (auto answer : board_list) {
            Sudoku::display_board(answer, false, i);
            i++;
        }
    }
};

} // namespace gudoku

#endif // GUDOKU_SUDOKU_H
