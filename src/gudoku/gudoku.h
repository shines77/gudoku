
#ifndef GUDOKU_SOLVER_H
#define GUDOKU_SOLVER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t gudoku_solver(const char * sudoku, char * soulution, size_t limit);

#ifdef __cplusplus
}
#endif

#endif // GUDOKU_SOLVER_H
