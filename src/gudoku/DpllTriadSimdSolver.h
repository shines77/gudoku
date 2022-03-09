
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
    union alignas(16) BandBoard {
        uint32_t bands[4];
        uint64_t bands64[2];

#if defined(WIN64) || defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) \
 || defined(_M_IA64) || defined(__amd64__) || defined(__x86_64__)
        void clear() {
            this->bands64[0] = 0;
            this->bands64[1] = 0;
        }

        void full() {
            this->bands64[0] = kBitSet27_Double64;
            this->bands64[1] = kBitSet27_Single64;
        }

        bool operator == (const BandBoard & rhs) {
            return (this->bands64[0] == rhs.bands64[0] && this->bands64[1] == rhs.bands64[1]);
        }

        bool operator != (const BandBoard & rhs) {
            return (this->bands64[0] != rhs.bands64[0] || this->bands64[1] != rhs.bands64[1]);
        }
#else
        void clear() {
            this->bands[0] = 0;
            this->bands[1] = 0;
            this->bands[2] = 0;
            this->bands[3] = 0;
        }

        void full() {
            this->bands[0] = kBitSet27;
            this->bands[1] = kBitSet27;
            this->bands[2] = kBitSet27;
            this->bands[3] = 0;
        }

        bool operator == (const BandBoard & rhs) {
            return (this->bands[0] == rhs.bands[0] &&
                    this->bands[1] == rhs.bands[1] &&
                    this->bands[2] == rhs.bands[2]);
        }

        bool operator == (const BandBoard & rhs) {
            return (this->bands[0] != rhs.bands[0] ||
                    this->bands[1] != rhs.bands[1] ||
                    this->bands[2] != rhs.bands[2]);
        }
#endif // __amd64__
    };

    struct alignas(32) State {
        BandBoard candidates[Numbers10];
        BandBoard prevCandidates[Numbers10];
        BandBoard solvedCells;
        BandBoard solvedRows;
        BandBoard pairs;
        BandBoard reserve;

        State() {
            /* Do nothing !! */
        }

        State(const State & src) {
            this->copy(src);
        }

        ~State() {
            /* Do nothing !! */
        }

        void init() {
#if defined(__AVX2__)
            BitVec16x16_AVX bitset27(kBitSet27_Double64, kBitSet27_Single64, kBitSet27_Double64, kBitSet27_Single64);
            BitVec16x16_AVX bitset27_half(kBitSet27_Double64, kBitSet27_Single64, 0, 0);
            BitVec16x16_AVX zeros;
            zeros.setAllZeros();
            {
                bitset27.saveAligned((void *)&this->candidates[0]);
                bitset27.saveAligned((void *)&this->candidates[2]);
                bitset27.saveAligned((void *)&this->candidates[4]);
                bitset27.saveAligned((void *)&this->candidates[6]);
                bitset27_half.saveAligned((void *)&this->candidates[8]);

                zeros.saveAligned((void *)&this->prevCandidates[0]);                
                zeros.saveAligned((void *)&this->prevCandidates[2]);               
                zeros.saveAligned((void *)&this->prevCandidates[4]);                
                zeros.saveAligned((void *)&this->prevCandidates[6]);
                zeros.saveAligned((void *)&this->prevCandidates[8]);
            }
            zeros.saveAligned((void *)&this->solvedCells);
            zeros.saveAligned((void *)&this->pairs);
#elif defined(__SSE2__)
            BitVec08x16 bitset27(kBitSet27_Double64, kBitSet27_Single64);
            BitVec08x16 zeros;
            zeros.setAllZeros();
            {
                bitset27.saveAligned((void *)&this->candidates[0]);
                bitset27.saveAligned((void *)&this->candidates[1]);
                bitset27.saveAligned((void *)&this->candidates[2]);
                bitset27.saveAligned((void *)&this->candidates[3]);
                bitset27.saveAligned((void *)&this->candidates[4]);
                bitset27.saveAligned((void *)&this->candidates[5]);
                bitset27.saveAligned((void *)&this->candidates[6]);
                bitset27.saveAligned((void *)&this->candidates[7]);
                bitset27.saveAligned((void *)&this->candidates[8]);
                zeros.saveAligned((void *)&this->candidates[9]);

                zeros.saveAligned((void *)&this->prevCandidates[0]);
                zeros.saveAligned((void *)&this->prevCandidates[1]);
                zeros.saveAligned((void *)&this->prevCandidates[2]);
                zeros.saveAligned((void *)&this->prevCandidates[3]);
                zeros.saveAligned((void *)&this->prevCandidates[4]);
                zeros.saveAligned((void *)&this->prevCandidates[5]);
                zeros.saveAligned((void *)&this->prevCandidates[6]);
                zeros.saveAligned((void *)&this->prevCandidates[7]);
                zeros.saveAligned((void *)&this->prevCandidates[8]);
                zeros.saveAligned((void *)&this->prevCandidates[9]);
            }
            zeros.saveAligned((void *)&this->solvedCells);
            zeros.saveAligned((void *)&this->pairs);
#else
            for (size_t num = 0; num < Numbers10; num++) {
                this->candidates[num].full();
                this->prevCandidates[num].clear();
            }
            this->solvedCells.clear();
            this->solvedRows.clear();
            this->pairs.clear();
#endif
        }

        void copy(const State & other) {
#if defined(__AVX2__)
            BitVec16x16_AVX B1, B2, B3, B4;
            BitVec08x16 B5;
            {
                B1.loadAligned((void *)&other.candidates[0]);
                B2.loadAligned((void *)&other.candidates[2]);
                B3.loadAligned((void *)&other.candidates[4]);
                B4.loadAligned((void *)&other.candidates[6]);
                B5.loadAligned((void *)&other.candidates[8]);
                B1.saveAligned((void *)&this->candidates[0]);
                B2.saveAligned((void *)&this->candidates[2]);
                B3.saveAligned((void *)&this->candidates[4]);
                B4.saveAligned((void *)&this->candidates[6]);
                B5.saveAligned((void *)&this->candidates[8]);

                B1.loadAligned((void *)&other.prevCandidates[0]);
                B2.loadAligned((void *)&other.prevCandidates[2]);
                B3.loadAligned((void *)&other.prevCandidates[4]);
                B4.loadAligned((void *)&other.prevCandidates[6]);
                B5.loadAligned((void *)&other.prevCandidates[8]);
                B1.saveAligned((void *)&this->prevCandidates[0]);
                B2.saveAligned((void *)&this->prevCandidates[2]);
                B3.saveAligned((void *)&this->prevCandidates[4]);
                B4.saveAligned((void *)&this->prevCandidates[6]);
                B5.saveAligned((void *)&this->prevCandidates[8]);
            }
            B1.loadAligned((void *)&other.solvedCells);
            B5.loadAligned((void *)&other.pairs);
            B1.saveAligned((void *)&this->solvedCells);
            B5.saveAligned((void *)&this->pairs);
#elif defined(__SSE2__)
            BitVec08x16 B1, B2, B3, B4;
            {
                B1.loadAligned((void *)&other.candidates[0]);
                B2.loadAligned((void *)&other.candidates[1]);
                B3.loadAligned((void *)&other.candidates[2]);
                B4.loadAligned((void *)&other.candidates[3]);
                B1.saveAligned((void *)&this->candidates[0]);
                B2.saveAligned((void *)&this->candidates[1]);
                B3.saveAligned((void *)&this->candidates[2]);
                B4.saveAligned((void *)&this->candidates[3]);

                B1.loadAligned((void *)&other.candidates[4]);
                B2.loadAligned((void *)&other.candidates[5]);
                B3.loadAligned((void *)&other.candidates[6]);
                B4.loadAligned((void *)&other.candidates[7]);
                B1.saveAligned((void *)&this->candidates[4]);
                B2.saveAligned((void *)&this->candidates[5]);
                B3.saveAligned((void *)&this->candidates[6]);
                B4.saveAligned((void *)&this->candidates[7]);

                B1.loadAligned((void *)&other.candidates[8]);
                B1.saveAligned((void *)&this->candidates[8]);

                B1.loadAligned((void *)&other.prevCandidates[0]);
                B2.loadAligned((void *)&other.prevCandidates[1]);
                B3.loadAligned((void *)&other.prevCandidates[2]);
                B4.loadAligned((void *)&other.prevCandidates[3]);
                B1.saveAligned((void *)&this->prevCandidates[0]);
                B2.saveAligned((void *)&this->prevCandidates[1]);
                B3.saveAligned((void *)&this->prevCandidates[2]);
                B4.saveAligned((void *)&this->prevCandidates[3]);

                B1.loadAligned((void *)&other.prevCandidates[4]);
                B2.loadAligned((void *)&other.prevCandidates[5]);
                B3.loadAligned((void *)&other.prevCandidates[6]);
                B4.loadAligned((void *)&other.prevCandidates[7]);
                B1.saveAligned((void *)&this->prevCandidates[4]);
                B2.saveAligned((void *)&this->prevCandidates[5]);
                B3.saveAligned((void *)&this->prevCandidates[6]);
                B4.saveAligned((void *)&this->prevCandidates[7]);

                B1.loadAligned((void *)&other.prevCandidates[8]);
                B1.saveAligned((void *)&this->prevCandidates[8]);
            }
            B1.loadAligned((void *)&other.solvedCells);
            B2.loadAligned((void *)&other.solvedRows);
            B3.loadAligned((void *)&other.pairs);
            B1.saveAligned((void *)&this->solvedCells);
            B2.saveAligned((void *)&this->solvedRows);            
            B3.saveAligned((void *)&this->pairs);
#else
            std::memcpy((void *)this, (const void *)&other, sizeof(State));
#endif
        }
    };

    State state_;

public:
    DpllTriadSimdSolver() : basic_solver() {}
    ~DpllTriadSimdSolver() {}

private:
    JSTD_FORCE_INLINE
    intptr_t init_sudoku(State & state, const char * puzzle) {
        state.init();

        BitVec08x16 solved_cells;
        solved_cells.setAllZeros();

        intptr_t candidates = 0;
        for (size_t pos = 0; pos < BoardSize; pos++) {
            unsigned char val = puzzle[pos];
            if (val != '.') {
                size_t num = val - '1';
                assert(num >= (Sudoku::kMinNumber - 1) && num <= (Sudoku::kMaxNumber - 1));
                //int validity = this->update_peer_cells(state, solved_cells, pos, num);
                int validity = 0;
                if (validity == Status::Invalid)
                    return (intptr_t)-1;
                candidates++;
            }
        }

        void * pCells16 = (void *)&state.solvedCells;
        solved_cells.saveAligned(pCells16);

        return candidates;
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
        intptr_t candidates = this->init_sudoku(state, puzzle);
        if (candidates < (intptr_t)Sudoku::kMinInitCandidates)
            return -1;

        this->reset_statistics(limit);


        return 0;
    }
};

} // namespace gudoku

#pragma pack(pop)

#endif // GUDOKU_DPLL_TRIAD_SIMD_SOLVER_H
