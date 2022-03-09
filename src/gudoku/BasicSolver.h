
#ifndef GUDOKU_BASIC_SOLVER_H
#define GUDOKU_BASIC_SOLVER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>

#include <cstdint>
#include <cstddef>
#include <vector>

#include "gudoku/Sudoku.h"

namespace gudoku {

class BasicSolver {
public:
    typedef BasicSolver this_type;

    static const size_t Rows = Sudoku::kRows;
    static const size_t Cols = Sudoku::kCols;
    static const size_t Boxes = Sudoku::kBoxes;
    static const size_t BoxSize = Sudoku::kBoxSize;
    static const size_t Numbers = Sudoku::kNumbers;

    static const size_t BoardSize = Sudoku::kBoardSize;
    static const size_t TotalSize = Sudoku::kTotalSize;

protected:
    size_t num_guesses_;

public:
    BasicSolver() : num_guesses_(0) {
    }
    ~BasicSolver() {}

    size_t get_num_guesses() { return this->num_guesses_; }

    void set_num_guesses(size_t num_guesses) {
        this->num_guesses_ = num_guesses;
    }

    static size_t calc_empties(const Board & board) {
        size_t empties = 0;
        for (size_t pos = 0; pos < BoardSize; pos++) {
            unsigned char val = board.cells[pos];
            if (val == '.') {
                empties++;
            }
        }
        return empties;
    }

    static void display_board(Board & board) {
        Sudoku::display_board(board, true);
    }

    template <size_t nSearchMode = SearchMode::OneSolution>
    void display_result(Board & board, double elapsed_time,
                        bool print_answer = true,
                        bool print_all_answers = true) {
        if (print_answer) {
            Sudoku::display_board(board);
        }
        printf("elapsed time: %0.3f ms, num_guesses: %" PRIuPTR "\n\n",
                elapsed_time, this->get_num_guesses());
    }
};

} // namespace gudoku

#endif // GUDOKU_BASIC_SOLVER_H
