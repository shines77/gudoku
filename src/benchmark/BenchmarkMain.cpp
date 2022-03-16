
#if defined(_MSC_VER) && !defined(NDEBUG)
#include <vld.h>
#endif

#if defined(_MSC_VER)
#define __MMX__
#define __SSE__
#define __SSE2__
#define __SSE3__
#define __SSSE3__
#define __SSE4A__
#define __SSE4a__
#define __SSE4_1__
#define __SSE4_2__
#define __POPCNT__
#define __LZCNT__
#define __AVX__
#define __AVX2__
#define __3dNOW__
#else
//#undef __SSE4_1__
//#undef __SSE4_2__
#endif

//#undef __SSE4_1__
//#undef __SSE4_2__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <cstring>      // For std::memset()
#include <vector>
#include <bitset>

#include <atomic>
#include <thread>
#include <chrono>

#include "gudoku/StopWatch.h"
#include "gudoku/CPUWarmUp.h"

#include "gudoku/gudoku.h"
#include "gudoku/BitUtils.h"

#include "gudoku/DpllTriadSimdSolver.h"

#include "gudoku/TestCase.h"
#include "gudoku/Sudoku.hpp"

using namespace gudoku;

static std::vector<Board> bm_puzzles;
static size_t bm_puzzleTotal = 0;

// Index: [0 - 4]
#define TEST_CASE_INDEX         4

void make_sudoku_board(Board & board, size_t index)
{
    for (size_t row = 0; row < Sudoku::Rows; row++) {
        size_t row_base = row * 9;
        size_t col = 0;
        const char * prows = test_case[index].rows[row];
        char val;
        while ((val = *prows) != '\0') {
            if (val >= '0' && val <= '9') {
                if (val != '0')
                    board.cells[row_base + col] = val;
                else
                    board.cells[row_base + col] = '.';
                col++;
                assert(col <= Sudoku::Cols);
            }
            else if (val == '.') {
                board.cells[row_base + col] = '.';
                col++;
                assert(col <= Sudoku::Cols);
            }
            prows++;
        }
        assert(col == Sudoku::Cols);
    }
}

size_t read_sudoku_board(Board & board, char line[256])
{
    char * pline = line;
    // Skip the white spaces
    while (*pline == ' ' || *pline == '\t') {
        pline++;
    }
    // Is a comment ?
    if ((*pline == '#') || ((*pline == '/') && (pline[1] = '/'))) {
        return 0;
    }

    size_t pos = 0;
    char val;
    while ((val = *pline++) != '\0') {
        if (val >= '0' && val <= '9') {
            if (val != '0')
                board.cells[pos] = val;
            else
                board.cells[pos] = '.';
            assert(pos < Sudoku::BoardSize);
            pos++;
            
        }
        else if ((val == '.') || (val == ' ') || (val == '-')) {
            board.cells[pos] = '.';
            assert(pos < Sudoku::BoardSize);
            pos++;
        }
    }
    assert(pos <= Sudoku::BoardSize);
    return pos;
}

size_t load_sudoku_puzzles(const char * filename, std::vector<Board> & puzzles)
{
    size_t puzzleCount = 0;

    std::ifstream ifs;
    try {
        ifs.open(filename, std::ios::in);
        if (ifs.good()) {
            ifs.seekg(0, std::ios::end);
            std::fstream::pos_type total_size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);

            //std::cout << std::endl;
            std::cout << "------------------------------------------" << std::endl << std::endl;
            std::cout << "File name: " << filename << std::endl;
            std::cout << "File size: " << total_size << " Byte(s)" << std::endl;

            size_t predictedSize = total_size / (Sudoku::BoardSize + 1) + 200;
            puzzles.resize(predictedSize);

            std::cout << "Predicted Size: " << predictedSize << std::endl << std::endl;

            while (!ifs.eof()) {
                char line[256];
                std::fill_n(line, sizeof(line), 0);
                ifs.getline(line, sizeof(line) - 1);

                Board board;
                board.clear();
                size_t num_grids = read_sudoku_board(board, line);
                // Sudoku::BoardSize = 81
                if (num_grids >= Sudoku::BoardSize) {
                    if (puzzleCount < predictedSize)
                        puzzles[puzzleCount] = board;
                    else
                        puzzles.push_back(board);
                    puzzleCount++;
                }
            }

            std::cout << "------------------------------------------" << std::endl << std::endl;

            ifs.close();
        }
    }
    catch (std::exception & ex) {
        std::cout << "Exception info: " << ex.what() << std::endl << std::endl;
    }

    return puzzleCount;
}

template <typename Slover>
void run_solver_testcase(size_t index)
{
    Board board, solution;
    board.clear();
    make_sudoku_board(board, index);

    Slover solver;
    solver.display_board(board);

    test::StopWatch sw;
    sw.start();
    size_t success = solver.solve(&board.cells[0], &solution.cells[0], 1);
    sw.stop();

    double elapsed_time = sw.getElapsedMillisec();
    solver.display_result(solution, elapsed_time);
}

void run_a_testcase(size_t index)
{
    test::CPU::WarmUp cpuWarnUp(1000);

    if (1)
    {
        printf("------------------------------------------\n\n");
        printf("gudoku: DpllTriadSimdSolver\n\n");

        run_solver_testcase<DpllTriadSimdSolver<1>>(index);
    }

    printf("------------------------------------------\n\n");
}

template <typename Solver>
void run_sudoku_test(std::vector<Board> & puzzles, size_t puzzleTotal, const char * name, const size_t limit)
{
    //printf("------------------------------------------\n\n");
    printf("gudoku: %s\n\n", name);

    size_t total_guesses = 0;
    size_t total_no_guess = 0;

    size_t puzzleCount = 0;
    size_t puzzleInvalid = 0;
    size_t puzzleSolved = 0;
    size_t puzzleMultiSolution = 0;
    double total_time = 0.0;

    Solver solver;

    Board solution;
    solution.clear();    

    test::StopWatch sw;
    sw.start();

    for (size_t i = 0; i < puzzleTotal; i++) {
        Board & board = puzzles[i];
        size_t solutions = solver.solve(&board.cells[0], &solution.cells[0], limit);
        if (solutions == 1) {
            size_t num_guesses = solver.get_num_guesses();
            total_guesses += num_guesses;
            total_no_guess += (num_guesses == 0);

            puzzleSolved++;
        }
        else if (solutions > 1) {
            puzzleMultiSolution++;
        }
        else {
            puzzleInvalid++;
        }
        puzzleCount++;
#ifdef _DEBUG
        if (puzzleCount > 100000)
            break;
#endif
    }

    sw.stop();
    total_time = sw.getElapsedMillisec();

    double no_guess_percent = calc_percent(total_no_guess, puzzleCount);

    printf("Total puzzle(s): %u / (%u solved, %u invalid, %u multi-solution).\n\n"
           "total_no_guess: %" PRIuPTR ", no_guess %% = %0.1f %%\n\n",
           (uint32_t)puzzleCount, (uint32_t)puzzleSolved, (uint32_t)puzzleInvalid, (uint32_t)puzzleMultiSolution,
           total_no_guess, no_guess_percent);
    printf("Total elapsed time: %0.3f ms, total_guesses: %" PRIuPTR "\n\n", total_time, total_guesses);

    if (puzzleCount != 0) {
        printf("%0.1f usec/puzzle, %0.2f guesses/puzzle, %0.1f puzzles/sec\n\n",
               total_time * 1000.0 / puzzleCount,
               (double)total_guesses / puzzleCount,
               puzzleCount / (total_time / 1000.0));
    }
    else {
        printf("NaN usec/puzzle, NaN guesses/puzzle, %0.1f puzzles/sec\n\n",
               puzzleCount / (total_time / 1000.0));
    }

    printf("------------------------------------------\n\n");
}

template <int LimitSolutions = 1>
void run_all_benchmark(const char * filename)
{
    // Read the puzzles data
    bm_puzzleTotal = load_sudoku_puzzles(filename, bm_puzzles);

    test::CPU::WarmUp cpuWarmUp(1000);
    
    static const int kSolutionMode = (LimitSolutions == 1) ? 1: 0;

#if !defined(_DEBUG)
    run_sudoku_test<DpllTriadSimdSolver<kSolutionMode>>(bm_puzzles, bm_puzzleTotal, "DpllTriadSimdSolver", LimitSolutions);
#else
    run_sudoku_test<DpllTriadSimdSolver<kSolutionMode>>(bm_puzzles, bm_puzzleTotal, "DpllTriadSimdSolver", LimitSolutions);
#endif
}

int main(int argc, char * argv[])
{
    const char * filename = nullptr;
    const char * out_file = nullptr;
    int limit_solution = 0;
    UNUSED_VARIANT(out_file);
    if (argc > 3) {
        filename = argv[1];
        limit_solution = atoi(argv[2]);
        out_file = argv[3];
    }
    else if (argc > 2) {
        filename = argv[1];
        limit_solution = atoi(argv[2]);
    }
    else if (argc > 1) {
        filename = argv[1];
    }

    Sudoku::initialize();

    if (1)
    {
        if (filename == nullptr) {
            run_a_testcase(TEST_CASE_INDEX);
        }
    }

    if (1)
    {
        if (filename != nullptr) {
            if (limit_solution <= 0 || limit_solution == 1)
                run_all_benchmark<1>(filename);
            else
                run_all_benchmark<2>(filename);
        }
    }

    Sudoku::finalize();

#if defined(_DEBUG) && defined(_MSC_VER)
    ::system("pause");
#endif

    return 0;
}
