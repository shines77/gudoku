
#ifndef GUDOKU_SUDOKU_TABLE_H
#define GUDOKU_SUDOKU_TABLE_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include <cstdint>
#include <cstddef>
#include <bitset>
#include <cstring>      // For std::memset(), std::memcpy()
#include <type_traits>
#include <algorithm>    // For std::sort(), std::fill_n()

#include <vector>
#include <atomic>

#include "gudoku/stddef.h"
#include "gudoku/BitSet.h"
#include "gudoku/BitArray.h"
#include "gudoku/BitVec.h"
#include "gudoku/Sudoku.h"

struct SudokuTable : public Sudoku {
    typedef typename Sudoku::board_type     board_type;
    typedef typename Sudoku::BitMask        BitMask;
    typedef typename Sudoku::BitMaskTable   BitMaskTable;

#pragma pack(push, 1)
    static const size_t NeighborsAlignBytes    = ((Neighbors * sizeof(uint8_t) + Alignment - 1) / Alignment) * Alignment;
    static const size_t NeighborsReserveBytes1 = NeighborsAlignBytes - Neighbors * sizeof(uint8_t);
    static const size_t NeighborsReserveBytes  = (NeighborsReserveBytes1 != 0) ? NeighborsReserveBytes1 : Alignment;

    // Aligned to sizeof(size_t) for cache friendly
    struct NeighborCells {
        uint8_t cells[Neighbors];
        uint8_t reserve[NeighborsReserveBytes];
    };

    struct CellInfo {
        uint8_t row, col;
        uint8_t box, cell;
        uint8_t box_pos;
        uint8_t box_base;
        uint8_t box_x, box_y;
        uint8_t cell_x, cell_y;
        // Reserve for cache friendly
        uint8_t reserve[6];
    };

    struct BoxesInfo {
        uint8_t row, col;
        uint8_t box, cell;
        uint8_t pos;
        uint8_t box_base;
        uint8_t box_x, box_y;
        uint8_t cell_x, cell_y;
        // Reserve for cache friendly
        uint8_t reserve[6];
    };
#pragma pack(pop)

    static bool is_inited;

    static CellInfo *       cell_info;
    static CellInfo *       cell_info16;
    static BoxesInfo *      boxes_info;
    static BoxesInfo *      boxes_info16;
    static NeighborCells *  neighbor_cells;
    static NeighborCells *  ordered_neighbor_cells;
    static BitMaskTable     neighbors_mask_tbl;

    SudokuTable() {
        SudokuTable::initialize();
    }

    ~SudokuTable() {
        SudokuTable::finalize();
    }

    static void initialize() {
        //
        // See: https://stackoverflow.com/questions/40579342/is-there-any-compiler-barrier-which-is-equal-to-asm-memory-in-c11
        //
        std::atomic_signal_fence(std::memory_order_release);        // _compile_barrier()
        if (!is_inited) {
            if (bPrintSudokuStaticInit)
                printf("SudokuTable::initialize()\n");
            neighbors_mask_tbl.reset();
            make_cell_info();
            make_boxes_info();
            make_neighbor_cells();
            is_inited = true;
        }
        std::atomic_signal_fence(std::memory_order_release);        // _compile_barrier()
    }

    static void finalize() {
        std::atomic_signal_fence(std::memory_order_release);        // _compile_barrier()
        if (is_inited) {
            if (bPrintSudokuStaticInit)
                printf("SudokuTable::finalize()\n");
            if (cell_info) {
                delete[] cell_info;
                cell_info = nullptr;
            }
            if (cell_info16) {
                delete[] cell_info16;
                cell_info16 = nullptr;
            }
            if (boxes_info) {
                delete[] boxes_info;
                boxes_info = nullptr;
            }
            if (boxes_info16) {
                delete[] boxes_info16;
                boxes_info16 = nullptr;
            }
            if (neighbor_cells) {
                delete[] neighbor_cells;
                neighbor_cells = nullptr;
            }
            if (ordered_neighbor_cells) {
                delete[] ordered_neighbor_cells;
                ordered_neighbor_cells = nullptr;
            }
            is_inited = false;
        }
        std::atomic_signal_fence(std::memory_order_release);        // _compile_barrier()
    }

    static void make_cell_info() {
        if (cell_info == nullptr) {
            cell_info = new CellInfo[BoardSize];
            cell_info16 = new CellInfo[Rows * Cols16];

            std::memset(cell_info, 0, sizeof(CellInfo) * BoardSize);
            std::memset(cell_info16, 0, sizeof(CellInfo) * Rows * Cols16);

            size_t pos = 0;
            for (size_t row = 0; row < Rows; row++) {
                for (size_t col = 0; col < Cols; col++) {
                    CellInfo * cellInfo = &cell_info[pos];
                    CellInfo * cellInfo16 = &cell_info16[row * Cols16 + col];

                    size_t box_x = col / BoxCellsX;
                    size_t box_y = row / BoxCellsY;
                    size_t box = box_y * BoxCountX + box_x;
                    size_t box_base = (box_y * BoxCellsY) * Cols + box_x * BoxCellsX;
                    size_t cell_x = col % BoxCellsX;
                    size_t cell_y = row % BoxCellsY;
                    size_t cell = cell_y * BoxCellsX + cell_x;
                    size_t box_pos = box * BoxSize + cell;

                    cellInfo->row = (uint8_t)row;
                    cellInfo->col = (uint8_t)col;
                    cellInfo->box = (uint8_t)box;
                    cellInfo->cell = (uint8_t)cell;
                    cellInfo->box_pos = (uint8_t)box_pos;
                    cellInfo->box_base = (uint8_t)box_base;
                    cellInfo->box_x = (uint8_t)box_x;
                    cellInfo->box_y = (uint8_t)box_x;
                    cellInfo->cell_x = (uint8_t)cell_x;
                    cellInfo->cell_y = (uint8_t)cell_y;

                    cellInfo16->row = (uint8_t)row;
                    cellInfo16->col = (uint8_t)col;
                    cellInfo16->box = (uint8_t)box;
                    cellInfo16->cell = (uint8_t)cell;
                    cellInfo16->box_pos = (uint8_t)box_pos;
                    cellInfo16->box_base = (uint8_t)box_base;
                    cellInfo16->box_x = (uint8_t)box_x;
                    cellInfo16->box_y = (uint8_t)box_x;
                    cellInfo16->cell_x = (uint8_t)cell_x;
                    cellInfo16->cell_y = (uint8_t)cell_y;

                    pos++;
                }
            }

            // print_cell_info();
        }
    }

    static void print_cell_info() {
        printf("    const uint8_t cell_info[%d][10] = {\n", (int)BoardSize);
        for (size_t pos = 0; pos < BoardSize; pos++) {
            printf("        {");
            const CellInfo & cellInfo = cell_info[pos];
            printf("%2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d",
                   (int)cellInfo.row, (int)cellInfo.col,
                   (int)cellInfo.box, (int)cellInfo.cell,
                   (int)cellInfo.box_pos, (int)cellInfo.box_base,
                   (int)cellInfo.box_x, (int)cellInfo.box_y,
                   (int)cellInfo.cell_x, (int)cellInfo.cell_y);
            if (pos < (BoardSize - 1))
                printf(" },  // %d\n", (int)pos);
            else
                printf(" }   // %d\n", (int)pos);
        }
        printf("    };\n\n");
    }

    static void make_boxes_info() {
        if (boxes_info == nullptr) {
            boxes_info = new BoxesInfo[Boxes * BoxSize];
            boxes_info16 = new BoxesInfo[Boxes * BoxSize16];

            std::memset(boxes_info, 0, sizeof(BoxesInfo) * Boxes * BoxSize);
            std::memset(boxes_info16, 0, sizeof(BoxesInfo) * Boxes * BoxSize16);

            size_t index = 0;
            for (size_t box = 0; box < Boxes; box++) {
                for (size_t cell = 0; cell < BoxSize; cell++) {
                    BoxesInfo * boxesInfo = &boxes_info[index];
                    BoxesInfo * boxesInfo16 = &boxes_info16[box * BoxSize16 + cell];

                    size_t row = (box / BoxCountX) * BoxCellsY + (cell / BoxCellsX);
                    size_t col = (box % BoxCountX) * BoxCellsX + (cell % BoxCellsX);
                    size_t pos = row * Cols + col;
                    size_t box_x = box % BoxCountX;
                    size_t box_y = box / BoxCountX;
                    size_t box_base = (box_y * BoxCellsY) * Cols + box_x * BoxCellsX;
                    size_t cell_x = col % BoxCellsX;
                    size_t cell_y = row % BoxCellsY;

                    boxesInfo->row = (uint8_t)row;
                    boxesInfo->col = (uint8_t)col;
                    boxesInfo->box = (uint8_t)box;
                    boxesInfo->cell = (uint8_t)cell;
                    boxesInfo->pos = (uint8_t)pos;
                    boxesInfo->box_base = (uint8_t)box_base;
                    boxesInfo->box_x = (uint8_t)box_x;
                    boxesInfo->box_y = (uint8_t)box_x;
                    boxesInfo->cell_x = (uint8_t)cell_x;
                    boxesInfo->cell_y = (uint8_t)cell_y;

                    boxesInfo16->row = (uint8_t)row;
                    boxesInfo16->col = (uint8_t)col;
                    boxesInfo16->box = (uint8_t)box;
                    boxesInfo16->cell = (uint8_t)cell;
                    boxesInfo16->pos = (uint8_t)pos;
                    boxesInfo16->box_base = (uint8_t)box_base;
                    boxesInfo16->box_x = (uint8_t)box_x;
                    boxesInfo16->box_y = (uint8_t)box_x;
                    boxesInfo16->cell_x = (uint8_t)cell_x;
                    boxesInfo16->cell_y = (uint8_t)cell_y;

                    index++;
                }
            }

            // print_boxes_info();
        }
    }

    static void print_boxes_info() {
        printf("    const uint8_t boxes_info[%d][10] = {\n", (int)(Boxes * BoxSize));
        for (size_t index = 0; index < BoardSize; index++) {
            printf("        {");
            const BoxesInfo & boxesInfo = boxes_info[index];
            printf("%2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d, %2d",
                   (int)boxesInfo.row, (int)boxesInfo.col,
                   (int)boxesInfo.box, (int)boxesInfo.cell,
                   (int)boxesInfo.pos, (int)boxesInfo.box_base,
                   (int)boxesInfo.box_x, (int)boxesInfo.box_y,
                   (int)boxesInfo.cell_x, (int)boxesInfo.cell_y);
            if (index < (Boxes * BoxSize - 1))
                printf(" },  // %d\n", (int)index);
            else
                printf(" }   // %d\n", (int)index);
        }
        printf("    };\n\n");
    }

    static size_t get_neighbor_cells_list(size_t row, size_t col,
                                          NeighborCells * list) {
        assert(list != nullptr);
        size_t index = 0;
        size_t pos_y = row * Cols;
        for (size_t x = 0; x < Cols; x++) {
            if (x != col) {
                list->cells[index++] = (uint8_t)(pos_y + x);
            }
        }

        size_t pos_x = col;
        for (size_t y = 0; y < Rows; y++) {
            if (y != row) {
                list->cells[index++] = (uint8_t)(y * Cols + pos_x);
            }
        }

        size_t box_x = col / BoxCellsX;
        size_t box_y = row / BoxCellsY;
        size_t box_base = (box_y * BoxCellsY) * Cols + box_x * BoxCellsX;
        size_t pos = pos_y + pos_x;
        size_t cell_x = col % BoxCellsX;
        size_t cell_y = row % BoxCellsY;
        size_t cell = box_base;
        for (size_t y = 0; y < BoxCellsY; y++) {
            if (y == cell_y) {
                cell += Cols;
            }
            else {
                for (size_t x = 0; x < BoxCellsX; x++) {
                    if (x != cell_x) {
                        assert(cell != pos);
                        list->cells[index++] = (uint8_t)(cell);
                    }
                    cell++;
                }
                cell += (Cols - BoxCellsX);
            }
        }

        assert(index == Neighbors);
        return index;
    }

    static void make_effect_mask(size_t pos) {
        NeighborCells * list = &neighbor_cells[pos];
        SmallBitSet<BoardSize> masks;
        for (size_t i = 0; i < Neighbors; i++) {
            size_t cell = list->cells[i];
            masks.set(cell);
        }
        neighbors_mask_tbl[pos] = masks;
    }

    static void make_neighbor_cells() {
        if (neighbor_cells == nullptr) {
            neighbor_cells = new NeighborCells[BoardSize];
            ordered_neighbor_cells = new NeighborCells[BoardSize];

            size_t pos = 0;
            for (size_t row = 0; row < Rows; row++) {
                for (size_t col = 0; col < Cols; col++) {
                    NeighborCells * list = &neighbor_cells[pos];
                    size_t neighbors = get_neighbor_cells_list(row, col, list);
                    assert(neighbors == Neighbors);
                    NeighborCells * ordered_list = &ordered_neighbor_cells[pos];
                    for (size_t cell = 0; cell < Neighbors; cell++) {
                        ordered_list->cells[cell] = list->cells[cell];
                    }
                    // Sort the cells for cache friendly
                    std::sort(&neighbor_cells[pos].cells[0], &neighbor_cells[pos].cells[Neighbors]);
                    make_effect_mask(pos);
                    pos++;
                }
            }

            // print_neighbor_cells();
            // print_ordered_neighbor_cells();
        }
    }

    static void print_neighbor_cells() {
        printf("    const uint8_t neighbor_cells[%d][%d] = {\n", (int)BoardSize, (int)Neighbors);
        for (size_t pos = 0; pos < BoardSize; pos++) {
            printf("        { ");
            for (size_t cell = 0; cell < Neighbors; cell++) {
                if (cell < Neighbors - 1)
                    printf("%2u, ", (uint32_t)neighbor_cells[pos].cells[cell]);
                else
                    printf("%2u ", (uint32_t)neighbor_cells[pos].cells[cell]);
            }
            if (pos < (BoardSize - 1))
                printf("},  // %d\n", (int)pos);
            else
                printf("}   // %d\n", (int)pos);
        }
        printf("    };\n\n");
    }

    static void print_ordered_neighbor_cells() {
        printf("    const uint8_t ordered_neighbor_cells[%d][%d] = {\n", (int)BoardSize, (int)Neighbors);
        for (size_t pos = 0; pos < BoardSize; pos++) {
            printf("        { ");
            for (size_t cell = 0; cell < Neighbors; cell++) {
                if (cell < Neighbors - 1)
                    printf("%2u, ", (uint32_t)ordered_neighbor_cells[pos].cells[cell]);
                else
                    printf("%2u ", (uint32_t)ordered_neighbor_cells[pos].cells[cell]);
            }
            if (pos < (BoardSize - 1))
                printf("},  // %d\n", (int)pos);
            else
                printf("}   // %d\n", (int)pos);
        }
        printf("    };\n\n");
    }
};

#endif // GUDOKU_SUDOKU_TABLE_H
