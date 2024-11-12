# gudoku

## 简介

本程序根据世界上最快的数独求解程序 [tdoku](https://github.com/t-dillon/tdoku) 改写，速度相当，但可以在 Windows 上的 VC++ 上编译。

特色：

`BitVec.h`: 一个跨平台，支持 SSE, SSE2, AVX, AVX2 和 AVX512 等 SIMD 指令的向量库。[点这里查看](./src/gudoku/BitVec.h)

包含以下类：

- BitVec08x16
- BitVec16x16_SSE
- BitVec16x16_AVX

对 SSE, SSE2, AVX, AVX2 和 AVX512 指令的常用操作做了比较完整的封装，主要服务于 `数独` 求解程序，但也尽量做了扩展。

## 关于 tudoku

tdoku 作者是 [t-dillon](https://github.com/t-dillon)，一个原理非常特别的数独求解程序，使用了 SIMD 技术，在求解较难的数独时速度非常快。

但在求解较简单的数独时，则是 Rust 写的 rust-sudoku 更快，我用 C++ 写的 [gz_sudoku](https://github.com/shines77/gz_sudoku/) 比 rust-sudoku 更快一些，约快 5% 。其中 rust-sudoku 和 [gz_sudoku](https://github.com/shines77/gz_sudoku/) 都是在国人 JCZ 写的 JCZSolve 的基础上修改而来的。

注：tdoku 只能在类 Linux 环境下编译。

Git: [https://github.com/t-dillon/tdoku](https://github.com/t-dillon/tdoku)

原理文档: [Nerd Sniped: A Sudoku Story](https://t-dillon.github.io/tdoku/)

## 测试集

数独的测试集放在 `./data` 目录下：

```bash
# 这两个测试集都是所有剩17个格子数独的集合，属于是比较简单的数独
# 这两个文件基本相同，但稍微有一点差异，只测试其中一个即可（建议）
./data/puzzles2_17_clue
./data/puzzles_17_clue_49151

# 这个是最难的 1905 个数独的测试集，属于是比较困难的数独
./data/puzzles5_forum_hardest_1905_11+
```

## 基准测试

### 1. 在 Linux 下：

请先选择或安装 gcc 和 clang 的不同版本。

- **gcc**

编译：

```bash
cmake .
make
```

运行测试：

```bash
./benchmark ./data/puzzles2_17_clue
./benchmark ./data/puzzles_17_clue_49151
./benchmark ./data/puzzles5_forum_hardest_1905_11+
```

- **clang**

请先切换到 ./clang 目录下，再用 CMake 来构建 makefile 和编译。

```bash
cd ./clang
cmake -G "Unix Makefiles" -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ ./
make
```

运行测试：

```bash
./benchmark ../data/puzzles2_17_clue
./benchmark ../data/puzzles_17_clue_49151
./benchmark ../data/puzzles5_forum_hardest_1905_11+
```

### 2. 在 Windows 下

请切换到 `.\bin\vc2015\x64-Release` 目录下，执行下面命令。

运行测试：

```bash
.\benchmark.exe ..\..\..\data\puzzles2_17_clue
.\benchmark.exe ..\..\..\data\puzzles_17_clue_49151
.\benchmark.exe ..\..\..\data\puzzles5_forum_hardest_1905_11+
```

## LeetCode

在 LeetCode 上，也有一道关于数独的题目：[LeetCode problem 37: SudokuSolver](https://leetcode.cn/problems/sudoku-solver/) 。
