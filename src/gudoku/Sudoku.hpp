
#include "gudoku/Sudoku.h"

using namespace gudoku;

/************ "Sudoku.h" *************/

bool Sudoku::is_inited = false;

Sudoku::CellInfo *
Sudoku::cell_info = nullptr;

Sudoku::CellInfo *
Sudoku::cell_info16 = nullptr;

Sudoku::BoxesInfo *
Sudoku::boxes_info = nullptr;

Sudoku::BoxesInfo *
Sudoku::boxes_info16 = nullptr;

Sudoku::NeighborCells *
Sudoku::neighbor_cells = nullptr;

Sudoku::NeighborCells *
Sudoku::ordered_neighbor_cells = nullptr;

Sudoku::BitMaskTable
Sudoku::neighbors_mask_tbl;
