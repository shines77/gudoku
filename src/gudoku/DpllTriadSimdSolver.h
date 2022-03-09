
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
struct Box {
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
struct Band {
    BitVec08x16 configurations;
    BitVec08x16 eliminations;

    Band() : configurations(kAll, kAll, kAll, kAll, kAll, kAll, 0, 0), eliminations(0, 0) {}
};

struct State {
    Band bands[2][3];
    Box  boxes[9];

    State() noexcept {}

    void init() {
        BitVec08x16 default_band(kAll, kAll, kAll, kAll, kAll, kAll, 0, 0);
        BitVec08x16 zeros;
        zeros.setAllZeros();
        bands[0][1].configurations = default_band;
        bands[0][1].eliminations = zeros;
        bands[0][2].configurations = default_band;
        bands[0][2].eliminations = zeros;
        bands[0][3].configurations = default_band;
        bands[0][3].eliminations = zeros;
        bands[1][1].configurations = default_band;
        bands[1][1].eliminations = zeros;
        bands[1][2].configurations = default_band;
        bands[1][2].eliminations = zeros;
        bands[1][3].configurations = default_band;
        bands[1][3].eliminations = zeros;

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
    // used when assigning a candidate during initialization
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
            {   0,   kAll,   kAll,   kAll,      0,   kAll,    0,    0},
            {kAll,      0,   kAll,   kAll,   kAll,      0,    0,    0},
            {kAll,   kAll,      0,      0,   kAll,   kAll,    0,    0},
            {   0,      0,      0,      0,      0,      0,    0,    0}
        },
        {
            {kAll,   kAll,      0,   kAll,   kAll,      0,    0,    0},
            {   0,   kAll,   kAll,      0,   kAll,   kAll,    0,    0},
            {kAll,      0,   kAll,   kAll,      0,   kAll,    0,    0},
            {   0,      0,      0,      0,      0,      0,    0,    0}
        },
        {
            {kAll,      0,   kAll,      0,   kAll,   kAll,    0,    0},
            {kAll,   kAll,      0,   kAll,      0,   kAll,    0,    0},
            {   0,   kAll,   kAll,   kAll,   kAll,      0,    0,    0},
            {   0,      0,      0,      0,      0,      0,    0,    0}
        }
    };

    // tables for constructing band elimination messages from BitVec08x16 containing
    // positive or negative triad views of a box stored positions 4, 5, and 6.
    // each table has three shuffle control vectors, one for each of the band's box
    // peers. there are three tables, each corresponding to a rotation of elements
    // in the peer. look first at the shift0 table to see the correspondence with
    // the configuration diagram reproduced above.
    //
    const BitVec08x16 triads_shift0_to_config_elims[3]{
        { shuf04, shuf05, shuf06, shuf06, shuf04, shuf05, 0xFFFF, 0xFFFF },
        { shuf05, shuf06, shuf04, shuf05, shuf06, shuf04, 0xFFFF, 0xFFFF },
        { shuf06, shuf04, shuf05, shuf04, shuf05, shuf06, 0xFFFF, 0xFFFF }
    };
    const BitVec08x16 triads_shift1_to_config_elims[3]{
        { shuf05, shuf06, shuf04, shuf04, shuf05, shuf06, 0xFFFF, 0xFFFF },
        { shuf06, shuf04, shuf05, shuf06, shuf04, shuf05, 0xFFFF, 0xFFFF },
        { shuf04, shuf05, shuf06, shuf05, shuf06, shuf04, 0xFFFF, 0xFFFF }
    };
    const BitVec08x16 triads_shift2_to_config_elims[3]{
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
    const BitVec16x16 shuffle_configs_to_triads[2]{
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
    const BitVec16x16 pos_triads_to_candidates[2][2]{
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

    const BitVec08x16 one_value_mask[9]{
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

    const uint8_t digitToCandidate[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 00
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 10
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 20
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,     // 30
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 40
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 50
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 60
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     // 70
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
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                triads_shift0_to_config_elims16[i * 3 + j] =
                        BitVec16x16{triads_shift0_to_config_elims[i],triads_shift0_to_config_elims[j]};
                triads_shift1_to_config_elims16[i * 3 + j] =
                        BitVec16x16{triads_shift1_to_config_elims[i],triads_shift1_to_config_elims[j]};
                triads_shift2_to_config_elims16[i * 3 + j] =
                        BitVec16x16{triads_shift2_to_config_elims[i],triads_shift2_to_config_elims[j]};
            }
        }

        for (uint32_t pos = 0; pos < 81; pos++) {
            box_indexing[pos] = BoxIndexing(pos);
        }
    }
};

const Tables tables {};

class DpllTriadSimdSolver : public BasicSolver {
public:
    typedef BasicSolver                         basic_solver;
    typedef DpllTriadSimdSolver                 this_type;

    typedef typename Sudoku::NeighborCells      NeighborCells;
    typedef typename Sudoku::CellInfo           CellInfo;
    typedef typename Sudoku::BoxesInfo          BoxesInfo;

    typedef typename Sudoku::BitMask            BitMask;
    typedef typename Sudoku::BitMaskTable       BitMaskTable;

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

    static const bool kAllDimIsSame = Sudoku::kAllDimIsSame;

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

public:
    DpllTriadSimdSolver() : basic_solver() {}
    ~DpllTriadSimdSolver() {}

private:
    static
    JSTD_FORCE_INLINE
    void init_clue(const char * puzzle, State & state, uint32_t pos) {
        const BoxIndexing & indexing = tables.box_indexing[pos];
        int8_t digit = (int8_t)puzzle[pos];
        assert(digit >= '0' && digit <= '9');
        uint16_t candidate = tables.digitToCandidate[digit];
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
    intptr_t init_sudoku(const char * puzzle, State & state) {
        state.init();

        uint64_t nonDotMask64 = whichIsNotDots64(puzzle);
        while (nonDotMask64 != 0) {
            uint32_t pos = BitUtils::bsf64(nonDotMask64);
            this_type::init_clue(puzzle, state, pos);
            nonDotMask64 = BitUtils::clearLowBit64(nonDotMask64);
        }

        return 0;
    }

public:
    inline
    void reset_statistics(size_t limit) {
        this->set_num_guesses(0);
        this->set_num_solutions(0);
        this->set_limit_solutions(limit);
    }

    JSTD_NO_INLINE
    size_t solve(const char * puzzle, char * solution, size_t limit) {
        State & state = this->state_;
        intptr_t candidates = this->init_sudoku(puzzle, state);
        if (candidates < (intptr_t)Sudoku::kMinInitCandidates)
            return -1;

        this->reset_statistics(limit);

        return 0;
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
