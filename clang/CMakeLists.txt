CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

project(gudoku)

set(CMAKE_VERBOSE_MAKEFILE ON)

message("-------------- Env ---------------")
message("  CMAKE_SOURCE_DIR:   ${CMAKE_SOURCE_DIR}")
message("  CMAKE_BINARY_DIR:   ${CMAKE_BINARY_DIR}")
message("  PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message("  PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")
message("----------------------------------")

if (NOT MSVC)
    ##
    ## See: https://www.cnblogs.com/stonehat/p/7000637.html
    ##
    ## Usage: (Ubuntu 16.04 Server)
    ##
    ##   apt-get install llvm clang clang-5.0 clang-6.0 clang-8
    ##

    ## For C_FLAGS
    ## -mmmx -msse -msse2 -msse3 -mssse3 -msse4 -msse4a -msse4.1 -msse4.2 -mavx -mavx2 -mavx512vl -mavx512f
    set(CMAKE_C_COMPILER                "/usr/bin/clang")
    set(CMAKE_C_FLAGS_DEFAULT           "${CMAKE_C_FLAGS} -std=c99 -march=native -Wall -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-variable -fPIC")
    set(CMAKE_C_FLAGS_RELEASE           "${CMAKE_C_FLAGS_DEFAULT} -O3 -DNDEBUG")
    set(CMAKE_C_FLAGS_DEBUG             "${CMAKE_C_FLAGS_DEFAULT} -g -pg -D_DEBUG")
    set(CMAKE_C_FLAGS_MINSIZEREL        "${CMAKE_C_FLAGS_DEFAULT} -Os -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO    "${CMAKE_C_FLAGS_DEFAULT} -O2 -g")


    ## For CXX_FLAGS
    ## -mmmx -msse -msse2 -msse3 -mssse3 -msse4 -msse4a -msse4.1 -msse4.2 -mavx -mavx2 -mavx512vl -mavx512f
    ## -Wall -Werror -Wextra -Wno-format -Wno-unused-function
    set(CMAKE_CXX_COMPILER              "/usr/bin/clang++")
    set(CMAKE_CXX_FLAGS_DEFAULT         "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libstdc++ -march=native -Wall -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-variable -fPIC")
    set(CMAKE_CXX_FLAGS_RELEASE         "${CMAKE_CXX_FLAGS_DEFAULT} -O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG           "${CMAKE_CXX_FLAGS_DEFAULT} -g -pg -D_DEBUG")
    set(CMAKE_CXX_FLAGS_MINSIZEREL      "${CMAKE_CXX_FLAGS_DEFAULT} -Os -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "${CMAKE_CXX_FLAGS_DEFAULT} -O2 -g")

    set(CMAKE_AR        "/usr/bin/llvm-ar")
    set(CMAKE_LINKER    "/usr/bin/llvm-link")
    set(CMAKE_NM        "/usr/bin/llvm-nm")
    set(CMAKE_OBJDUMP   "/usr/bin/llvm-objdump")
    set(CMAKE_RANLIB    "/usr/bin/llvm-ranlib")

    set(CMAKE_EXE_LINKER_FLAGS "-lstdc++")

    ##
    ## See: https://cloud.tencent.com/developer/ask/101584
    ##
    ## "/usr/bin/c++", "/usr/bin/g++", "/usr/bin/clang++"
    message("  CMAKE_C_COMPILER:   ${CMAKE_C_COMPILER}")
    message("  CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        # using Clang
        if (CONAN_LIBCXX STREQUAL "libstdc++11")
            add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)
        elseif (CONAN_LIBCXX STREQUAL "libstdc++")
            add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        # using GCC
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
        # using Intel C++
    elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
        # using Visual Studio C++
    endif()
endif()

if (MSVC AND MSVC_STATIC_CRT)
    set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            )
    foreach (CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

if (WIN32)
    add_compile_options("-D_WIN32_WINNT=0x0601")
    add_compile_options("-D_CRT_SECURE_NO_WARNINGS")
    ## set(EXTRA_LIBS ${EXTRA_LIBS} ws2_32 mswsock)
endif()

if (UNIX)
    set(EXTRA_LIBS ${EXTRA_LIBS} pthread)
else()
    set(EXTRA_LIBS ${EXTRA_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()

include_directories(../include)
include_directories(../src)

set(SOURCE_FILES
    ../src/gudoku/SudokuMain.cpp
    )

add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target all
    COMMENT "Switch CMAKE_BUILD_TYPE to Release"
)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message("------------ Options -------------")
message("  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("----------------------------------")

add_executable(gudoku ${SOURCE_FILES})
target_link_libraries(gudoku ${EXTRA_LIBS})

project(benchmark)

include_directories(../include)
include_directories(../src)
include_directories(../src/benchmark)

set(SOURCE_FILES
    ../src/benchmark/BenchmarkMain.cpp
    )

add_executable(benchmark ${SOURCE_FILES})
target_link_libraries(benchmark ${EXTRA_LIBS})
