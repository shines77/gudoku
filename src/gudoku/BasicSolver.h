
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

    static size_t num_guesses;

public:
    BasicSolver() {
        //this->init_statistics();
    }
    ~BasicSolver() {}

    static size_t get_num_guesses() { return this_type::num_guesses; }

private:
    void init_statistics() {
        num_guesses = 0;
    }

public:
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
    static void display_result(Board & board, double elapsed_time,
                               bool print_answer = true,
                               bool print_all_answers = true) {
        if (print_answer) {
            Sudoku::display_board(board);
        }
        printf("elapsed time: %0.3f ms, num_guesses: %" PRIuPTR "\n\n",
                elapsed_time, this_type::get_num_guesses());
    }
};

} // namespace gudoku

#endif // GUDOKU_BASIC_SOLVER_H
