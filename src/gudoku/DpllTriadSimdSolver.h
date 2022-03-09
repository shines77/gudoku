
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

namespace gudoku {

class DpllTriadSimdSolver : public BasicSolver {
public:
    intptr_t solve() {
        //
        return 0;
    }
};

} // namespace gudoku

#endif // GUDOKU_DPLL_TRIAD_SIMD_SOLVER_H
