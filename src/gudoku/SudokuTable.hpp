
#include "gudoku/SudokuTable.h"

using namespace gudoku;

/************ "SudokuTable.h" *************/

bool SudokuTable::is_inited = false;

SudokuTable::CellInfo *
SudokuTable::cell_info = nullptr;

SudokuTable::CellInfo *
SudokuTable::cell_info16 = nullptr;

SudokuTable::BoxesInfo *
SudokuTable::boxes_info = nullptr;

SudokuTable::BoxesInfo *
SudokuTable::boxes_info16 = nullptr;

SudokuTable::NeighborCells *
SudokuTable::neighbor_cells = nullptr;

SudokuTable::NeighborCells *
SudokuTable::ordered_neighbor_cells = nullptr;

SudokuTable::BitMaskTable
SudokuTable::neighbors_mask_tbl;
