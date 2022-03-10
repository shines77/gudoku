
#include "gudoku/gudoku.h"

#ifndef GUDOKU_NO_MAIN
#if defined(GUDOKU_SOLVER) || defined(STATIC_LIB) || defined(SHARED_DLL)
#define GUDOKU_NO_MAIN  1
#else
#define GUDOKU_NO_MAIN  0
#endif
#endif // GUDOKU_NO_MAIN

#if (GUDOKU_NO_MAIN != 0)

#include "gudoku/CPUWarmUp.h"
#include "gudoku/StopWatch.h"

#include "gudoku/gudoku.h"
#include "gudoku/BitUtils.h"

#include "gudoku/DpllTriadSimdSolver.h"

#include "gudoku/Sudoku.hpp"

using namespace gudoku;

namespace gudoku {
    static DpllTriadSimdSolver dpllTriadSimdSolver;
}

size_t gudoku_solver(const char * sudoku, char * solution,
                     size_t limit, size_t * num_guesses)
{
    size_t solutions = dpllTriadSimdSolver.solve(sudoku, solution, limit);
    *num_guesses = dpllTriadSimdSolver.get_num_guesses();
    return solutions;
}

#else // (GUDOKU_NO_MAIN == 0)

size_t gudoku_solver(const char * sudoku, char * solution,
                     size_t limit, size_t * num_guesses)
{
    return 0;
}

#endif // (GUDOKU_NO_MAIN != 0)
