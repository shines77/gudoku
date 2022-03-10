
#ifndef GUDOKU_DPLL_TRIAD_SIMD_SOLVER_H
#define GUDOKU_DPLL_TRIAD_SIMD_SOLVER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <cstring>      // For std::memset(), std::memcpy()
#include <vector>
#include <bitset>
#include <array>        // For std::array<T, Size>

#include "gudoku/BasicSolver.h"
#include "gudoku/Sudoku.h"
#include "gudoku/BitUtils.h"
#include "gudoku/BitSet.h"
#include "gudoku/BitArray.h"
#include "gudoku/BitVec.h"

#pragma pack(push, 1)

namespace gudoku {

static const size_t kSearchMode = SearchMode::OneSolution;

static const uint16_t kAll   = 0x01FF;
static const uint32_t kAll32 = 0x01FF01FFU;
static const uint64_t kAll64 = 0x01FF01FF01FF01FFULL;

static const size_t kHorizontal = 0;
static const size_t kVertical = 1;

//  The state of each box is stored in a vector of 16 uint16_t,     +---+---+---+---+
//  arranged as a 4x4 matrix of 9-bit candidate sets (the high      | c | c | c | H |
//  7 bits of each value are always zero). The top-left 3x3 sub-    +---+---+---+---+
//  matrix stores candidate sets for the 9 cells("c") of the box,   | c | c | c | H |
//  while the right 3x1 column and bottom 1x3 row store candidate   +---+---+---+---+
//  sets representing negative horizontal("H") and vertical("V")    | c | c | c | H |
//  triads respectively. A negative triad candidate will be         +---+---+---+---+
//  eliminated whenever we know that the same value must exist      | V | V | V |   |
//  in one three regular cells to which the triad corresponds.      +---+---+---+---+
//
//  For each value bit there is an exactly-one constraint over the 4 cells in a row
//  or column of the matrix corresponding to the biconditional defining the triad.
//
//  Each cell also has a minimum. So there are three sets of clauses represented here.
//
struct alignas(32) Box {
    BitVec16x16 cells;

    Box() : cells(BitVec16x16::full16(kAll)) {}
};

// For a given value there are only 6 possible configurations for how that value can be
// placed in the triads of a band. Our primary representation for the state of a band will
// be in terms of these configurations rather than the triads themselves. The possible
// configurations are numbered according to the following table:
//
//            config       0       1       2       3       4       5
//             elem      0 1 2   0 1 2   0 1 2   0 1 2   0 1 2   0 1 2
//                     +-------+-------+-------+-------+-------+-------+
//            peer0    | X . . | . X . | . . X | . . X | X . . | . X . |
//            peer1    | . X . | . . X | X . . | . X . | . . X | X . . |
//            peer2    | . . X | X . . | . X . | X . . | . X . | . . X |
//                     +-------+-------+-------+-------+-------+-------+
//
// The primary state of the band is stored as 9-bit masks     elem    0   1   2
// in the first 6 elements of an 8 uint16_t vector.                 +---+---+---+---+
//                                                           peer0  | t | t | t |   |
// When constructing elimination masks to send to the boxes         +---+---+---+---+
// we'll convert the configuration vector into a 3x3 matrix  peer1  | t | t | t |   |
// of positive triad candidates, which are arranged with            +---+---+---+---+
// box peers along the rows of 4x4 matrix in a 16 uint16_t   peer2  | t | t | t |   |
// vector (for both horizontal and vertical bands).                 +---+---+---+---+
//                                                                  |   |   |   |   |
// We'll also store with the Band a vector of eliminations          +---+---+---+---+
// to be applied to the Band's configurations on the next
// call to BandEliminate. This allows us to apply all pending updates to a band at
// the first opportunity instead of individually depending on where in the call stack
// the update originates.
//
// Note that we might do the same thing for Boxes, and doing so is beneficial for easy
// puzzles. Unfortunately, it's a net loss for hard puzzles. The cost in State size
// is higher, and the benefit is lower (the benefit for Bands chiefly arises from the
// way we do puzzle initialization).
//
struct alignas(32) Band {
    BitVec08x16 configurations;
    BitVec08x16 eliminations;

    Band() : configurations(kAll, kAll, kAll, kAll, kAll, kAll, 0, 0), eliminations(0, 0) {}
};

struct alignas(32) State {
    Band bands[2][3];
    Box  boxes[9];

    State() noexcept {}

    void init() {
#if 1
        BitVec16x16_AVX default_band(kAll32, kAll32, kAll32, 0, 0, 0, 0, 0);
        default_band.saveAligned((void *)&bands[0][0].configurations);
        default_band.saveAligned((void *)&bands[0][1].configurations);
        default_band.saveAligned((void *)&bands[0][2].configurations);
        default_band.saveAligned((void *)&bands[1][0].configurations);
        default_band.saveAligned((void *)&bands[1][1].configurations);
        default_band.saveAligned((void *)&bands[1][2].configurations);

        BitVec16x16_AVX default_box(kAll32, kAll32, kAll32, kAll32, kAll32, kAll32, kAll32, kAll32);
        default_box.saveAligned((void *)&boxes[0].cells);
        default_box.saveAligned((void *)&boxes[1].cells);
        default_box.saveAligned((void *)&boxes[2].cells);
        default_box.saveAligned((void *)&boxes[3].cells);
        default_box.saveAligned((void *)&boxes[4].cells);
        default_box.saveAligned((void *)&boxes[5].cells);
        default_box.saveAligned((void *)&boxes[6].cells);
        default_box.saveAligned((void *)&boxes[7].cells);
        default_box.saveAligned((void *)&boxes[8].cells);
#else
        BitVec08x16 default_band(kAll, kAll, kAll, kAll, kAll, kAll, 0, 0);
        BitVec08x16 zeros;
        zeros.setAllZeros();
        bands[0][0].configurations = default_band;
        bands[0][0].eliminations = zeros;
        bands[0][1].configurations = default_band;
        bands[0][1].eliminations = zeros;
        bands[0][2].configurations = default_band;
        bands[0][2].eliminations = zeros;
        bands[1][0].configurations = default_band;
        bands[1][0].eliminations = zeros;
        bands[1][1].configurations = default_band;
        bands[1][1].eliminations = zeros;
        bands[1][2].configurations = default_band;
        bands[1][2].eliminations = zeros;

        BitVec16x16 default_box(kAll32, kAll32, kAll32, kAll32, kAll32, kAll32, kAll32, kAll32);
        boxes[0].cells = default_box;
        boxes[1].cells = default_box;
        boxes[2].cells = default_box;
        boxes[3].cells = default_box;
        boxes[4].cells = default_box;
        boxes[5].cells = default_box;
        boxes[6].cells = default_box;
        boxes[7].cells = default_box;
        boxes[8].cells = default_box;
#endif
    }
};

struct BoxIndexing {
    static const size_t BoxCellsX = Sudoku::kBoxCellsX;      // 3
    static const size_t BoxCellsY = Sudoku::kBoxCellsY;      // 3
    static const size_t BoxCountX = Sudoku::kBoxCountX;      // 3
    static const size_t BoxCountY = Sudoku::kBoxCountY;      // 3
    static const size_t MinNumber = Sudoku::kMinNumber;      // 1
    static const size_t MaxNumber = Sudoku::kMaxNumber;      // 9

    static const size_t Rows = Sudoku::kRows;
    static const size_t Cols = Sudoku::kCols;
    static const size_t Boxes = Sudoku::kBoxes;
    static const size_t BoxSize = Sudoku::kBoxSize;
    static const size_t Numbers = Sudoku::kNumbers;

    static const size_t BoardSize = Sudoku::kBoardSize;
    static const size_t TotalSize = Sudoku::kTotalSize;

    uint8_t box_x;
    uint8_t box_y;
    uint8_t box;
    uint8_t cell_x;
    uint8_t cell_y;
    uint8_t cell;
    uint8_t reserve1;
    uint8_t reserve2;

    BoxIndexing() noexcept {}

    explicit BoxIndexing(uint32_t pos) noexcept
        : box_x((uint8_t)((pos % Cols) / BoxCountX)), box_y((uint8_t)(pos / (Cols * BoxCellsY))),
          box((uint8_t)(box_y * BoxCountX + box_x)),
          cell_x((uint8_t)(pos % BoxCellsX)), cell_y((uint8_t)((pos / Cols) % BoxCellsY)),
          cell((uint8_t)(cell_y * (BoxCellsX + 1) + cell_x)),
          reserve1((uint8_t)0), reserve2((uint8_t)0) {}
};

//
// We depend on low-level shuffle operations that address packed 8-bit integers, but we're
// always shuffling 16-bit logical cells. These constants are used for constructing shuffle
// control vectors that address these cells. We only require 8 of them since even 256-bit
// shuffles operate within 128-bit lanes.
//
static const uint16_t shuf00 = 0x0100, shuf01 = 0x0302, shuf02 = 0x0504, shuf03 = 0x0706;
static const uint16_t shuf04 = 0x0908, shuf05 = 0X0B0A, shuf06 = 0X0D0C, shuf07 = 0X0F0E;

struct Tables {
    // @formatter:off
    // Used when assigning a candidate during initialization
    BitVec16x16 cell_assignment_eliminations[9][16];

    //   config       0       1       2       3       4       5
    //    elem      0 1 2   0 1 2   0 1 2   0 1 2   0 1 2   0 1 2
    //            +-------+-------+-------+-------+-------+-------+
    //   peer0    | X . . | . X . | . . X | . . X | X . . | . X . |
    //   peer1    | . X . | . . X | X . . | . X . | . . X | X . . |
    //   peer2    | . . X | X . . | . X . | X . . | . X . | . . X |
    //            +-------+-------+-------+-------+-------+-------+
    //
    // A set of masks for eliminating band configurations inconsistent with the placement
    // of a digit in an element (minirow or minicol) of a box peer.
    //
    const BitVec08x16 peer_x_elem_to_config_mask[3][4] = {
        {
            {    0,   kAll,   kAll,   kAll,      0,   kAll,    0,    0 },
            { kAll,      0,   kAll,   kAll,   kAll,      0,    0,    0 },
            { kAll,   kAll,      0,      0,   kAll,   kAll,    0,    0 },
            {    0,      0,      0,      0,      0,      0,    0,    0 }
        },                                                             
        {                                                              
            { kAll,   kAll,      0,   kAll,   kAll,      0,    0,    0 },
            {    0,   kAll,   kAll,      0,   kAll,   kAll,    0,    0 },
            { kAll,      0,   kAll,   kAll,      0,   kAll,    0,    0 },
            {    0,      0,      0,      0,      0,      0,    0,    0 }
        },                                                             
        {                                                              
            { kAll,      0,   kAll,      0,   kAll,   kAll,    0,    0 },
            { kAll,   kAll,      0,   kAll,      0,   kAll,    0,    0 },
            {    0,   kAll,   kAll,   kAll,   kAll,      0,    0,    0 },
            {    0,      0,      0,      0,      0,      0,    0,    0 }
        }
    };

    // tables for constructing band elimination messages from BitVec08x16 containing
    // positive or negative triad views of a box stored positions 4, 5, and 6.
    // each table has three shuffle control vectors, one for each of the band's box
    // peers. there are three tables, each corresponding to a rotation of elements
    // in the peer. look first at the shift0 table to see the correspondence with
    // the configuration diagram reproduced above.
    //
    const BitVec08x16 triads_shift0_to_config_elims[3] = {
        { shuf04, shuf05, shuf06, shuf06, shuf04, shuf05, 0xFFFF, 0xFFFF },
        { shuf05, shuf06, shuf04, shuf05, shuf06, shuf04, 0xFFFF, 0xFFFF },
        { shuf06, shuf04, shuf05, shuf04, shuf05, shuf06, 0xFFFF, 0xFFFF }
    };
    const BitVec08x16 triads_shift1_to_config_elims[3] = {
        { shuf05, shuf06, shuf04, shuf04, shuf05, shuf06, 0xFFFF, 0xFFFF },
        { shuf06, shuf04, shuf05, shuf06, shuf04, shuf05, 0xFFFF, 0xFFFF },
        { shuf04, shuf05, shuf06, shuf05, shuf06, shuf04, 0xFFFF, 0xFFFF }
    };
    const BitVec08x16 triads_shift2_to_config_elims[3] = {
        { shuf06, shuf04, shuf05, shuf05, shuf06, shuf04, 0xFFFF, 0xFFFF },
        { shuf04, shuf05, shuf06, shuf04, shuf05, shuf06, 0xFFFF, 0xFFFF },
        { shuf05, shuf06, shuf04, shuf06, shuf04, shuf05, 0xFFFF, 0xFFFF }
    };

    // BitVec16x16 shuffle control vectors constructed from the 9 pairings of 3x3 vectors in
    // the tables above (because this makes access more efficient in AssertionsToEliminations).
    BitVec16x16 triads_shift0_to_config_elims16[9] {};
    BitVec16x16 triads_shift1_to_config_elims16[9] {};
    BitVec16x16 triads_shift2_to_config_elims16[9] {};

    // two BitVec16x16 shuffle control vectors whose results are or'ed together to convert
    // a vector of configurations (reproduced across 128 bit lanes) into a 3x3 matrix of
    // positive triads (refer again to the configuration diagram above).
    const BitVec16x16 shuffle_configs_to_triads[2] = {
        {
            { shuf00, shuf01, shuf02, 0xFFFF, shuf02, shuf00, shuf01, 0xFFFF },
            { shuf01, shuf02, shuf00, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }
        },
        {
            { shuf04, shuf05, shuf03, 0xFFFF, shuf05, shuf03, shuf04, 0xFFFF },
            { shuf03, shuf04, shuf05, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }
        }
    };

    // two pairs of two BitVec16x16 shuffle control vectors whose results are or'ed together to
    // convert vectors of positive triads in positions 0, 1, and 2 (reproduced across 128 bit
    // lanes) into box candidate sets. it is necessary to combine two shuffles because box
    // negative triads are eliminated when band positive triads have been eliminated in the
    // other two shifted positions. the shuffled input has 0xFFFF in position 3 to allow a
    // no-op for triads with opposite orientation.
    const BitVec16x16 pos_triads_to_candidates[2][2] = {
        // horizontal
        {
            {
                { shuf00, shuf00, shuf00, shuf01, shuf01, shuf01, shuf01, shuf02 },
                { shuf02, shuf02, shuf02, shuf00, shuf03, shuf03, shuf03, shuf03 }
            },
            {
                { shuf00, shuf00, shuf00, shuf02, shuf01, shuf01, shuf01, shuf00 },
                { shuf02, shuf02, shuf02, shuf01, shuf03, shuf03, shuf03, shuf03 }
            }
        },
        // vertical
        {
            {
                { shuf00, shuf01, shuf02, shuf03, shuf00, shuf01, shuf02, shuf03 },
                { shuf00, shuf01, shuf02, shuf03, shuf01, shuf02, shuf00, shuf03 }
            },
            {
                { shuf00, shuf01, shuf02, shuf03, shuf00, shuf01, shuf02, shuf03 },
                { shuf00, shuf01, shuf02, shuf03, shuf02, shuf00, shuf01, shuf03 }
            }
        }
    };

    const BitVec16x16 cell3x3_mask = BitVec16x16(
        kAll, kAll, kAll,    0,
        kAll, kAll, kAll,    0,
        kAll, kAll, kAll,    0,
            0,   0,    0,    0
    );

    // row rotation shuffle controls vectors for just the 3x3 submatrix of a BitVec16x16
    const BitVec16x16 row_rotate_3x3_1 = BitVec16x16(
        shuf01, shuf02, shuf00, shuf03, shuf05, shuf06, shuf04, shuf07,
        shuf01, shuf02, shuf00, shuf03, shuf04, shuf05, shuf06, shuf07
    );

    const BitVec16x16 row_rotate_3x3_2 = BitVec16x16(
        shuf02, shuf00, shuf01, shuf03, shuf06, shuf04, shuf05, shuf07,
        shuf02, shuf00, shuf01, shuf03, shuf04, shuf05, shuf06, shuf07
    );

    const BitVec08x16 one_value_mask[9] = {
        BitVec08x16::full16(1u << 0u), BitVec08x16::full16(1u << 1u), BitVec08x16::full16(1u << 2u),
        BitVec08x16::full16(1u << 3u), BitVec08x16::full16(1u << 4u), BitVec08x16::full16(1u << 5u),
        BitVec08x16::full16(1u << 6u), BitVec08x16::full16(1u << 7u), BitVec08x16::full16(1u << 8u),
    };

    const int box_peers[2][3][3] = {
        {
            { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 }
        },
        {
            { 0, 3, 6 }, { 1, 4, 7 }, { 2, 5, 8 }
        }
    };
    const int div3[9] = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
    const int mod3[9] = { 0, 1, 2, 0, 1, 2, 0, 1, 2 };

    const uint32_t box_base_tbl[9] = {
        0, 3, 6, 27, 30, 33, 54, 57, 60
    };
    const uint16_t digit_to_candidate[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 00
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 20
        0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 0, 0, 0, 0, 0,     // 30
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 40
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 50
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 60
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 70
    };

    const char bitmask_to_digit[512] = {
        '0', '1', '2',  0 , '3',  0 ,  0 ,  0 , '4',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 000
        '5',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 010
        '6',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 020
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 030
        '7',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 040
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 050
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 060
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 070

        '8',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 080
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 090
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 0A0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 0B0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 0C0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 0D0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 0E0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 0F0

        '9',  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 100
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 110
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 120
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 130
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 140
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 150
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 160
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 170

         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 180
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 190
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 1A0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 1B0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 1C0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 1D0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 1E0
         0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,  0 ,     // 1F0
    };

    BoxIndexing box_indexing[81];

    Tables() noexcept {
        for (int i : { 0, 1, 2, 4, 5, 6, 8, 9, 10 }) {
            // only need for cells, not triads
            for (uint32_t value = 0; value < 9; value++) {
                BitVec16x16 & mask = cell_assignment_eliminations[value][i];
                for (int j = 0; j < 15; j++) {
                    if (j == i) {
                        // asserted cell: clear all bits but the one asserted
                        mask.insert(j, kAll ^ (1u << value));
                    }
                    else if (j / 4 < 3 && j % 4 < 3) {
                        // conflict cell: clear the asserted bit
                        mask.insert(j, 1u << value);
                    }
                    else if (j / 4 == i / 4 || j % 4 == i % 4) {
                        // clear 2 negative triads
                        mask.insert(j, 1u << value);
                    }
                }
            }
        }

        for (int x = 0; x < Sudoku::kBoxCountX; x++) {
            for (int y = 0; y < Sudoku::kBoxCountY; y++) {
                int box_idx = x * Sudoku::kBoxCountY + y;
                triads_shift0_to_config_elims16[box_idx] =
                        BitVec16x16{triads_shift0_to_config_elims[x], triads_shift0_to_config_elims[y]};
                triads_shift1_to_config_elims16[box_idx] =
                        BitVec16x16{triads_shift1_to_config_elims[x], triads_shift1_to_config_elims[y]};
                triads_shift2_to_config_elims16[box_idx] =
                        BitVec16x16{triads_shift2_to_config_elims[x], triads_shift2_to_config_elims[y]};
            }
        }

        for (uint32_t pos = 0; pos < 81; pos++) {
            box_indexing[pos] = BoxIndexing(pos);
        }
    }
};

const Tables tables {};

class alignas(32) DpllTriadSimdSolver : public BasicSolver {
public:
    typedef BasicSolver                 basic_solver;
    typedef DpllTriadSimdSolver         this_type;

    static const size_t kAlignment = Sudoku::kAlignment;
    static const size_t BoxCellsX = Sudoku::kBoxCellsX;      // 3
    static const size_t BoxCellsY = Sudoku::kBoxCellsY;      // 3
    static const size_t BoxCountX = Sudoku::kBoxCountX;      // 3
    static const size_t BoxCountY = Sudoku::kBoxCountY;      // 3
    static const size_t MinNumber = Sudoku::kMinNumber;      // 1
    static const size_t MaxNumber = Sudoku::kMaxNumber;      // 9

    static const size_t Rows = Sudoku::kRows;
    static const size_t Cols = Sudoku::kCols;
    static const size_t Boxes = Sudoku::kBoxes;
    static const size_t BoxSize = Sudoku::kBoxSize;
    static const size_t Numbers = Sudoku::kNumbers;

    static const size_t BoardSize = Sudoku::kBoardSize;
    static const size_t TotalSize = Sudoku::kTotalSize;
    static const size_t Neighbors = Sudoku::kNeighbors;

    static const size_t Rows16 = Sudoku::kRows16;
    static const size_t Cols16 = Sudoku::kCols16;
    static const size_t Numbers10 = Sudoku::kNumbers10;
    static const size_t Numbers16 = Sudoku::kNumbers16;
    static const size_t Boxes16 = Sudoku::kBoxes16;
    static const size_t BoxSize16 = Sudoku::kBoxSize16;
    static const size_t BoardSize16 = Sudoku::kBoardSize16;

    static const size_t kAllRowBits = Sudoku::kAllRowBits;
    static const size_t kAllColBits = Sudoku::kAllColBits;
    static const size_t kAllBoxBits = Sudoku::kAllBoxBits;
    static const size_t kAllBoxCellBits = Sudoku::kAllBoxCellBits;
    static const size_t kAllNumberBits = Sudoku::kAllNumberBits;

    // all pencil marks set - 27 bits per band
    static const uint32_t kBitSet27          = 0x07FFFFFFUL;
    static const uint64_t kBitSet27_Single64 = 0x0000000007FFFFFFULL;
    static const uint64_t kBitSet27_Double64 = 0x07FFFFFF07FFFFFFULL;

    static const uint32_t kFullRowBits   = 0x01FFUL;
    static const uint32_t kFullRowBits_1 = 0x01FFUL << 9U;
    static const uint32_t kFullRowBits_2 = 0x01FFUL << 18U;

    static const uint32_t kBand0RowBits  = 0007;
    static const uint32_t kBand1RowBits  = 0070;
    static const uint32_t kBand2RowBits  = 0700;

private:
    State state_;
    State result_state_;

public:
    DpllTriadSimdSolver() : basic_solver() {}
    ~DpllTriadSimdSolver() {}

private:
    // Restrict the cell, minirow, and minicol clauses of the box to contain only the given
    // cell and triad candidates.
    template <int from_vertical>
    static bool boxRestrict(State & state, int box_idx, const BitVec16x16 & candidates) {
        // Return immediately if there are no new eliminations
        Box & box = state.boxes[box_idx];
        if (box.cells.isSubsetOf(candidates)) return true;
        auto eliminating = box.cells.and_not(candidates);

        int box_x = tables.mod3[box_idx];
        int box_y = tables.div3[box_idx];

        Band & h_band = state.bands[0][box_y];
        Band & v_band = state.bands[1][box_x];
        do {
            // Apply eliminations and check that no cell clause now violates its minimum
            box.cells = box.cells.and_not(eliminating);
            BitVec16x16 counts = box.cells.popcount16<16, Numbers>();
            const BitVec16x16 box_minimums(1, 1, 1, 6, 1, 1, 1, 6, 1, 1, 1, 6, 6, 6, 6, 0);
            if (counts.hasAnyLessThan(box_minimums)) return false;

            // Gather literals asserted by triggered cell clauses
            BitVec16x16 triggered = counts.whichIsEqual(box_minimums);
            BitVec16x16 all_assertions = box.cells & triggered;
            // And add literals asserted by triggered triad definition clauses
            gatherTriadClauseAssertions(
                    box.cells, [](const BitVec16x16 & x) { return x.rotateRows(); }, all_assertions);
            gatherTriadClauseAssertions(
                    box.cells, [](const BitVec16x16 & x) { return x.rotateCols(); }, all_assertions);

            // Construct elimination messages for this box and for our band peers
            assertionsToEliminations(all_assertions, box_x, box_y, eliminating,
                                     h_band.eliminations, v_band.eliminations);

        } while (eliminating.hasIntersects(box.cells));

        // Send elimination messages to horizontal and vertical peers. Prefer to send the first
        // of these messages to the peer whose orientation is opposite that of the inbound peer.
        if (from_vertical) {
            return (bandEliminate<kHorizontal>(state, box_y, box_x) &&
                    bandEliminate<kVertical>  (state, box_x, box_y));
        } else {
            return (bandEliminate<kVertical>  (state, box_x, box_y) &&
                    bandEliminate<kHorizontal>(state, box_y, box_x));
        }
    }

    // input BitVec16x16 contains zeros where nothing is asserted, a single candidate for regular cells
    // that are being asserted, and either 1 or 6 candidates for negative triad literals that are
    // being asserted (due to an unsatisfiable triad definition, or due to a 6/ minimum).
    static inline void assertionsToEliminations(const BitVec16x16 & assertions, int box_x, int box_y,
                                                BitVec16x16 & box_eliminations,
                                                BitVec08x16 & h_band_eliminations,
                                                BitVec08x16 & v_band_eliminations) {
        //
        // We could guard some or all of the code below with checks that the assertion vector as
        // a whole, or cell or negative triad components of it, are nonzero. but the branches
        // would most often be taken. it's cheaper to compute no-op updates than it is to pay the
        // branch cost.
        //

        // Update the self eliminations for new assertions in the box.
        auto cell_assertions_only = assertions & tables.cell3x3_mask;
        // Compute matrices broadcasting assertions across rows and columns in which they occur.
        BitVec16x16 across_rows = cell_assertions_only;
        across_rows |= across_rows.rotateRows();
        across_rows |= across_rows.rotateRows2();
        BitVec16x16 across_cols = cell_assertions_only;
        across_cols |= across_cols.rotateCols();
        across_cols |= across_cols.rotateCols2();
        // Let 3x3 submatrix have assertions occuring anywhere
        BitVec16x16 new_box_eliminations = BitVec16x16::X_or_Y_or_Z(across_cols,
                                                            across_cols.shuffle(tables.row_rotate_3x3_1),
                                                            across_cols.shuffle(tables.row_rotate_3x3_2));
        // Join 3x3 submatrix, row/col margins, and all elimination bits in asserted cells
        new_box_eliminations = BitVec16x16::X_or_Y_or_Z(
                new_box_eliminations, across_rows, cell_assertions_only.whichIsNonZero());
        // Then apply after clearing elimination bits for the asserted candidates.
        box_eliminations = BitVec16x16::X_xor_Y_or_Z(
                new_box_eliminations, cell_assertions_only, box_eliminations);

        // Below we'll update band eliminations to reflect assertion of negative triads or positive
        // literals within this box. in the case of asserted negative triads we'll eliminate the
        // corresponding positive triads in the band (at shift 0).
        BitVec16x16 hv_neg_triad_assertions(
            horizontalTriads(assertions),
            verticalTriads(assertions)
        );
        // in the case of asserted positive literals, which imply the assertion of corresponding
        // shift 0 positive triads, we'll eliminate the triads at shifts 1 and 2 in the band.
        BitVec16x16 hv_pos_triad_assertions(
            horizontalTriads(new_box_eliminations),
            verticalTriads(new_box_eliminations)
        );
        BitVec16x16 new_eliminations = BitVec16x16::X_or_Y_or_Z(
                hv_neg_triad_assertions.shuffle(
                        tables.triads_shift0_to_config_elims16[box_x * BoxCountY + box_y]),
                hv_pos_triad_assertions.shuffle(
                        tables.triads_shift1_to_config_elims16[box_x * BoxCountY + box_y]),
                hv_pos_triad_assertions.shuffle(
                        tables.triads_shift2_to_config_elims16[box_x * BoxCountY + box_y]));
        h_band_eliminations |= new_eliminations.low;
        v_band_eliminations |= new_eliminations.high;
    }

    // Extracts a BitVec08x16 containing (positive or negative) vertical triad literals in positions
    // 4, 5, and 6 for use in shuffling an elimination message to send the vertical band peer. the
    // contents of other cells are ignored by the shuffle.
    static inline BitVec08x16 verticalTriads(const BitVec16x16 & cells) {
        return cells.high;
    }

    // Extracts a BitVec08x16 containing (positive or negative) horizontal triad literals in positions
    // 4, 5, and 6 for use in shuffling an elimination message to send the horizontal band peer. we
    // use positions 4,5,6 so we can use the same tables in creating horizontal and vertical
    // elimination messages (and so the vertical triads can be extracted at no cost).
    static inline BitVec08x16 horizontalTriads(const BitVec16x16 & cells) {
        BitVec16x16 split_triads = cells.shuffle(
                BitVec16x16{{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, shuf03, shuf07, 0xFFFF, 0xFFFF},
                            {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, shuf03, 0xFFFF}});
        return (split_triads.low | split_triads.high);
    }

    template <typename RotateFn>
    static inline
    void gatherTriadClauseAssertions(const BitVec16x16 & cells,
                                     RotateFn rotate, BitVec16x16 & assertions) {
        // Find 'one_or_more' and 'two_or_more', each a set of 4 row/col vectors depending on the
        // given rotation function, where each cell in a row/col contains the bits that occur 1+ or
        // 2+ times across the cells of the corresponding source row/col.
        auto one_or_more = cells;
        auto rotated = rotate(cells);
        auto two_or_more = one_or_more & rotated;
        one_or_more |= rotated;
        rotated = rotate(rotated);
        two_or_more = BitVec16x16::X_and_Y_or_Z(one_or_more, rotated, two_or_more);
        one_or_more |= rotated;
        rotated = rotate(rotated);
        two_or_more = BitVec16x16::X_and_Y_or_Z(one_or_more, rotated, two_or_more);
        // we might rotate again and check that one_or_more == kAll, but the check is a net loss.
        // now assert (in cells where they remain) candidates that occur only once an a row/col.
        assertions = BitVec16x16::X_andnot_Y_or_Z(cells, two_or_more, assertions);
    }

    template <int vertical>
    static bool bandEliminate(State & state, int band_idx, int from_peer = 0) {
        Band & band = state.bands[vertical][band_idx];
        if (likely(!band.configurations.hasIntersects(band.eliminations))) return true;
        // after eliminating we might check that every value is still consistent with some
        // configuration, but the check is a net loss.
        band.configurations = band.configurations.and_not(band.eliminations);

        BitVec16x16 triads = configurationsToPositiveTriads(band.configurations);
        // we might check here that every cell (corresponding to a minirow or minicol) still has
        // at least three triad candidates, but the check is a net loss.
        BitVec16x16 counts = triads.popcount16<16, Numbers>();

        // we might repeat the updating of triads below until we no longer trigger new triad 3/
        // clauses. however, just once delivers most of the benefit, and it's best not to branch.
        BitVec16x16 asserting = triads & counts.whichIsEqual(BitVec16x16::full16(3));
        BitVec08x16 low  = asserting.low;
        BitVec08x16 high = asserting.high;
        band.configurations = band.configurations.and_not(BitVec08x16::X_or_Y_or_Z(
                low.rotateCols().shuffle(tables.triads_shift1_to_config_elims[0]),
                low.rotateCols().shuffle(tables.triads_shift2_to_config_elims[0]),
                low.shuffle(tables.triads_shift1_to_config_elims[1])));
        band.configurations = band.configurations.and_not(BitVec08x16::X_or_Y_or_Z(
                low.shuffle(tables.triads_shift2_to_config_elims[1]),
                high.rotateCols().shuffle(tables.triads_shift1_to_config_elims[2]),
                high.rotateCols().shuffle(tables.triads_shift2_to_config_elims[2])));
        triads = configurationsToPositiveTriads(band.configurations);

        // convert positive triads to box restriction messages and send to the three box peers.
        // send these messages in order so that we return to the inbound peer last.
        int peer[3] = { tables.mod3[from_peer + 1], tables.mod3[from_peer + 2], from_peer };
        auto & box_peers = tables.box_peers[vertical][band_idx];
        BitVec08x16 peer_triads[3]{ triads.low, triads.low.rotateCols(), triads.high };
        return (boxRestrict<vertical>(state, box_peers[peer[0]],
                        positiveTriadsToBoxCandidates<vertical>(peer_triads[peer[0]])) &&
                boxRestrict<vertical>(state, box_peers[peer[1]],
                        positiveTriadsToBoxCandidates<vertical>(peer_triads[peer[1]])) &&
                boxRestrict<vertical>(state, box_peers[peer[2]],
                        positiveTriadsToBoxCandidates<vertical>(peer_triads[peer[2]])));
    }

    // Convert band configuration into an equivalent 3x3 matrix of positive triad candidates,
    // where each row represents the constraints the band imposes on a given box peer.
    static inline BitVec16x16 configurationsToPositiveTriads(const BitVec08x16 & configurations) {
        BitVec16x16 tmp{configurations, configurations};
        return (tmp.shuffle(tables.shuffle_configs_to_triads[0]) |
                tmp.shuffle(tables.shuffle_configs_to_triads[1]));
    }

    // Convert 3 sets of positive triads (found in cells 0,1,2 of the given BitVec08x16) into a
    // mask for restricting the corresponding box peer.
    template <int vertical>
    static inline BitVec16x16 positiveTriadsToBoxCandidates(const BitVec08x16 & triads) {
        BitVec08x16 triads_with_kAll = triads | BitVec08x16{0x0, 0x0, 0x0, kAll, 0x0, 0x0, 0x0, 0x0};
        BitVec16x16 tmp{triads_with_kAll, triads_with_kAll};
        return (tmp.shuffle(tables.pos_triads_to_candidates[vertical][0]) |
                tmp.shuffle(tables.pos_triads_to_candidates[vertical][1]));
    }

    ///////////////////////////////////////////////////////////////////////////////////

    static const uint32_t NONE = UINT32_MAX;

    static inline
    std::pair<uint32_t, BitVec08x16>
    chooseBandAndValueToBranch(const State & state) {
        uint32_t best_band = NONE, best_band_count = NONE;
        uint32_t best_value = NONE, best_value_count = NONE;

        //
        // First find the unfixed band with the fewest possible configurations across all values.
        // a minimum unfixed band will have 0 <= count-10 <= 44. if all bands are fixed then the
        // minimum after subtracting 10 and interpreting as a uint will be 0xfffa.
        //
        uint32_t config_minpos = BitVec08x16(
                (uint16_t)state.bands[0][0].configurations.popcount(),
                (uint16_t)state.bands[0][1].configurations.popcount(),
                (uint16_t)state.bands[0][2].configurations.popcount(),
                (uint16_t)state.bands[1][0].configurations.popcount(),
                (uint16_t)state.bands[1][1].configurations.popcount(),
                (uint16_t)state.bands[1][2].configurations.popcount(),
                (uint16_t)0xFFFF,
                (uint16_t)0xFFFF
        ).minPosGreaterThanOrEqual<10>();

        // if we have an unfixed band then find a digit in the band with the fewest possibilities.
        // the approach below is faster than actually counting the configuration for each digit and
        // using MinPosGreaterThanOrEqual as above, but it is inexact in rare cases when all digits
        // have 4 or more configurations. the tradeoff is a net positive for Vanilla Sudoku and a
        // net negative for Pencilmark Sudoku.
        if ((config_minpos & 0xFF00u) == 0) {
            best_band = config_minpos >> 16u;
            const auto & configurations =
                    state.bands[tables.div3[best_band]][tables.mod3[best_band]].configurations;
            BitVec08x16 one = configurations;
            BitVec08x16 shuffle_rotate = BitVec08x16(shuf01, shuf02, shuf03, shuf04, shuf05, shuf00, 0xFFFF, 0xFFFF);
            BitVec08x16 rotated = one.shuffle(shuffle_rotate); // 1
            BitVec08x16 two = one & rotated;
            one |= rotated;
            rotated = rotated.shuffle(shuffle_rotate); // 2
            BitVec08x16 three = two & rotated;
            two |= one & rotated;
            one |= rotated;
            rotated = rotated.shuffle(shuffle_rotate); // 3
            BitVec08x16 four = three & rotated;
            three |= two & rotated;
            two |= one & rotated;
            one |= rotated;
            rotated = rotated.shuffle(shuffle_rotate); // 4
            four |= three & rotated;
            three |= two & rotated;
            two |= one & rotated;
            one |= rotated;
            rotated = rotated.shuffle(shuffle_rotate); // 5
            four |= three & rotated;
            three |= two & rotated;
            two |= one & rotated;

            BitVec08x16 only_two = two.and_not(three);
            if (!likely(only_two.isAllZeros())) {
                return { best_band, only_two.getLowBit() };
            } else {
                BitVec08x16 only_three = three.and_not(four);
                if (likely(!only_three.isAllZeros())) {
                    return { best_band, only_three.getLowBit() };
                } else {
                    return { best_band, four.getLowBit() };
                }
            }
        }
        return { best_band, BitVec08x16::full16(0) };
    }

    template <int vertical>
    void branchOnBandAndValue(int band_idx, const BitVec08x16 & value_mask, State & state) {
        Band & band = state.bands[vertical][band_idx];
        // We enter with two or more possible configurations for this value
        BitVec08x16 value_configurations = band.configurations & value_mask;
        // Assign the first configuration by eliminating the others
        this->num_guesses_++;
        State next_state = state;
        BitVec08x16 assignment_elims = value_configurations.clearLowBit();
        next_state.bands[vertical][band_idx].eliminations |= assignment_elims;
        if (bandEliminate<vertical>(next_state, band_idx)) {
            countSolutionsConsistentWithPartialAssignment(next_state);
            if (this->num_solutions_ >= this->limit_solutions_) return;
        }
        // now negate the first configuration
        BitVec08x16 negation_elims = value_configurations ^ assignment_elims;
        state.bands[vertical][band_idx].eliminations |= negation_elims;
        if (bandEliminate<vertical>(state, band_idx)) {
            countSolutionsConsistentWithPartialAssignment(state);
        }
    }

    //
    // Do not call this twice on the same state. for efficiency this count may modify the
    // given state instead of making copies. if called with limit > 1 this can leave the state
    // changed in a way that makes subsequent calls return different results.
    //
    void countSolutionsConsistentWithPartialAssignment(State & state) {
        auto band_and_value = chooseBandAndValueToBranch(state);
        if (band_and_value.first == NONE) {
            this->num_solutions_++;
            if (this->num_solutions_ == this->limit_solutions_)
                this->result_state_ = state;
        } else {
            if (band_and_value.first < 3) {
                branchOnBandAndValue<kHorizontal>(
                        tables.mod3[band_and_value.first], band_and_value.second, state);
            } else {
                branchOnBandAndValue<kVertical>(
                        tables.mod3[band_and_value.first], band_and_value.second, state);
            }
        }
    }

    size_t safeCountSolutionsConsistentWithPartialAssignment(State state, size_t limit) {
        this->num_solutions_ = 0;
        this->limit_solutions_ = limit;
        countSolutionsConsistentWithPartialAssignment(state);
        return this->num_solutions_;
    }

    static
    JSTD_FORCE_INLINE
    void initClue(const char * puzzle, State & state, uint32_t pos) {
        const BoxIndexing & indexing = tables.box_indexing[pos];
        int8_t digit = (int8_t)puzzle[pos];
        assert(digit >= '1' && digit <= '9');
        uint16_t candidate = tables.digit_to_candidate[digit];
        assert(candidate == (uint16_t)(1U << (uint32_t)(digit - '1')));
        //
        // Perform eliminations for the clue in its own box, but don't propagate. this is
        // not strictly necessary since band eliminations will constrain the puzzle, but it
        // turns out to be important for performance on invalid zero-solution puzzles.
        //
        state.boxes[indexing.box].cells = state.boxes[indexing.box].cells.and_not(
                tables.cell_assignment_eliminations[digit - '1'][indexing.cell]);

        // Merge band eliminations; we'll propagate after all clue are processed.
        state.bands[0][indexing.box_y].eliminations = BitVec08x16::X_and_Y_or_Z(
                tables.peer_x_elem_to_config_mask[indexing.box_x][indexing.cell_y],
                BitVec08x16::full16(candidate),
                state.bands[0][indexing.box_y].eliminations);

        state.bands[1][indexing.box_x].eliminations = BitVec08x16::X_and_Y_or_Z(
                tables.peer_x_elem_to_config_mask[indexing.box_y][indexing.cell_x],
                BitVec08x16::full16(candidate),
                state.bands[1][indexing.box_x].eliminations);
    }

    static
    JSTD_FORCE_INLINE
    bool initSudoku(const char * puzzle, State & state) {
        state.init();

        uint64_t nonDotMask64 = whichIsNotDots64(puzzle);
        while (nonDotMask64 != 0) {
            uint32_t pos = BitUtils::bsf64(nonDotMask64);
            initClue(puzzle, state, pos);
            nonDotMask64 = BitUtils::clearLowBit64(nonDotMask64);
        }       

        uint32_t nonDotMask16 = whichIsNotDots16(puzzle + 64);
        while (nonDotMask16 != 0) {
            uint32_t pos = BitUtils::bsf32(nonDotMask16);
            initClue(puzzle, state, pos + 64);
            nonDotMask16 = BitUtils::clearLowBit32(nonDotMask16);
        }

        if (puzzle[80] != '.') {
            initClue(puzzle, state, 80);
        }

        //
        // Thanks to the merging of band updates the puzzle is almost always fully initialized
        // after the first of these calls. most will be no-ops, but we've still got to do them
        // since this cannot be guaranteed.
        //
        return (bandEliminate<kHorizontal>(state, 0, 1) && bandEliminate<kVertical>(state, 0, 1) &&
                bandEliminate<kHorizontal>(state, 1, 2) && bandEliminate<kVertical>(state, 1, 2) &&
                bandEliminate<kHorizontal>(state, 2, 0) && bandEliminate<kVertical>(state, 2, 0));
    }

    static
    JSTD_FORCE_INLINE
    bool initSudoku(const char * puzzle, State & state, size_t & out_candidates) {
        state.init();

        uint64_t nonDotMask64 = whichIsNotDots64(puzzle);
        size_t candidates = BitUtils::popcnt64(nonDotMask64);
        while (nonDotMask64 != 0) {
            uint32_t pos = BitUtils::bsf64(nonDotMask64);
            initClue(puzzle, state, pos);
            nonDotMask64 = BitUtils::clearLowBit64(nonDotMask64);
        }       

        uint32_t nonDotMask16 = whichIsNotDots16(puzzle + 64);
        candidates += BitUtils::popcnt32(nonDotMask16);
        while (nonDotMask16 != 0) {
            uint32_t pos = BitUtils::bsf32(nonDotMask16);
            initClue(puzzle, state, pos + 64);
            nonDotMask16 = BitUtils::clearLowBit32(nonDotMask16);
        }

        if (puzzle[80] != '.') {
            candidates++;
            initClue(puzzle, state, 80);
        }

        out_candidates = candidates;

        //
        // Thanks to the merging of band updates the puzzle is almost always fully initialized
        // after the first of these calls. most will be no-ops, but we've still got to do them
        // since this cannot be guaranteed.
        //
        return (bandEliminate<kHorizontal>(state, 0, 1) && bandEliminate<kVertical>(state, 0, 1) &&
                bandEliminate<kHorizontal>(state, 1, 2) && bandEliminate<kVertical>(state, 1, 2) &&
                bandEliminate<kHorizontal>(state, 2, 0) && bandEliminate<kVertical>(state, 2, 0));
    }

public:
    static inline
    void extractMiniRow(uint64_t minirow, uint32_t minirow_base, char * solution) {
        solution[minirow_base + 0] = tables.bitmask_to_digit[uint16_t((minirow >> 0u ) & 0xFFFF)];
        solution[minirow_base + 1] = tables.bitmask_to_digit[uint16_t((minirow >> 16u) & 0xFFFF)];
        solution[minirow_base + 2] = tables.bitmask_to_digit[uint32_t( minirow >> 32u) & 0xFFFFU];
    }

    static
    void extractSolution(const State & state, char * solution) {
        for (int box_idx = 0; box_idx < 9; box_idx++) {
            const Box & box = state.boxes[box_idx];
            IntVec256 box_minirows;
            box.cells.saveAligned((void *)&box_minirows);
            uint32_t box_base = tables.box_base_tbl[box_idx];
            assert(box_base == (tables.div3[box_idx] * 27 + tables.mod3[box_idx] * 3));
            extractMiniRow(box_minirows.u64[0], box_base,      solution);
            extractMiniRow(box_minirows.u64[1], box_base + 9,  solution);
            extractMiniRow(box_minirows.u64[2], box_base + 18, solution);
        }
    }

    inline
    void resetStatistics(size_t limit) {
        this->set_num_guesses(0);
        this->set_num_solutions(0);
        this->set_limit_solutions(limit);
    }

    JSTD_NO_INLINE
    size_t solve(const char * puzzle, char * solution, size_t limit) {
        this->resetStatistics(limit);

        State & state = this->state_;
#if 1
        bool success = this->initSudoku(puzzle, state);
        if (success) {
            countSolutionsConsistentWithPartialAssignment(state);
            extractSolution(this->result_state_, solution);
        }
#else
        size_t candidates;
        bool success = this->initSudoku(puzzle, state, candidates);
        if (success && (candidates >= Sudoku::kMinInitCandidates)) {
            countSolutionsConsistentWithPartialAssignment(state);
            extractSolution(this->result_state_, solution);
        }
#endif
        return this->num_solutions_;
    }

    void display_result(Board & board, double elapsed_time,
                        bool print_answer = true,
                        bool print_all_answers = true) {
        basic_solver::display_result<kSearchMode>(board, elapsed_time, print_answer, print_all_answers);
    }
};

} // namespace gudoku

#pragma pack(pop)

#endif // GUDOKU_DPLL_TRIAD_SIMD_SOLVER_H
