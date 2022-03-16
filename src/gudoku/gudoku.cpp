
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

using namespace gudoku;

namespace gudoku {
DpllTriadSimdSolver<0> solver_none{};
DpllTriadSimdSolver<1> solver_last{};
}

size_t gudoku_solver(const char * sudoku, char * solution, uint32_t configuration,
                     size_t limit, size_t * num_guesses)
{
    bool return_last = (limit == 1 || configuration > 0);
    size_t solutions;
    if (return_last) {
        solutions = solver_last.solve(sudoku, solution, limit);
        *num_guesses = solver_last.get_num_guesses();
    }
    else {
        solutions = solver_none.solve(sudoku, solution, limit);
        *num_guesses = solver_none.get_num_guesses();
    }
    
    return solutions;
}

#else // (GUDOKU_NO_MAIN == 0)

size_t gudoku_solver(const char * sudoku, char * solution, uint32_t configuration,
                     size_t limit, size_t * num_guesses)
{
    return 0;
}

#endif // (GUDOKU_NO_MAIN != 0)
