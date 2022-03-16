
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

class alignas(32) BasicSolver {
public:
    typedef BasicSolver this_type;

    static const size_t Rows = Sudoku::Rows;
    static const size_t Cols = Sudoku::Cols;
    static const size_t Boxes = Sudoku::Boxes;
    static const size_t BoxSize = Sudoku::BoxSize;
    static const size_t Numbers = Sudoku::Numbers;

    static const size_t BoardSize = Sudoku::BoardSize;
    static const size_t TotalSize = Sudoku::TotalSize;

protected:
    size_t num_guesses_;
    size_t num_solutions_;
    size_t limit_solutions_;
    size_t reserve1_;

public:
    BasicSolver() : num_guesses_(0), num_solutions_(0), limit_solutions_(0), reserve1_(0) {
    }
    ~BasicSolver() {}

    size_t get_num_guesses() const { return this->num_guesses_; }
    size_t get_num_solutions() const { return this->num_solutions_; }
    size_t get_limit_solutions() const { return this->limit_solutions_; }

    void set_num_guesses(size_t num_guesses) {
        this->num_guesses_ = num_guesses;
    }
    void set_num_solutions(size_t num_solutions) {
        this->num_solutions_ = num_solutions;
    }
    void set_limit_solutions(size_t limit_solutions) {
        this->limit_solutions_ = limit_solutions;
    }

    void inc_num_guesses() {
        this->num_guesses_++;
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
