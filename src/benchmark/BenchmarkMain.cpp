
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

#include "gudoku/BasicSolver.hpp"
#include "gudoku/Sudoku.hpp"

using namespace gudoku;

int main(int argc, char * argv [])
{
    test::CPU::WarmUp cpuWarnUp(1000);

    printf("gudoku benchmark ver 1.0\n");

    DpllTriadSimdSolver solver;
    solver.solve();

    return 0;
}
