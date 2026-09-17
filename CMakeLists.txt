cmake_minimum_required(VERSION 3.14)

project(basic C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

set(core_sources
    src/err.c
    src/str.c
    src/lex.c
    src/exec.c
    src/prog.c
    src/var.c
    src/expr.c
    src/fmt.c
    src/math.c)

set(core_entry src/run.c)

set(core_flags
    -Wall -Wextra -Wdouble-promotion -Wfloat-conversion
    -Wshadow -Wmissing-prototypes -Wstrict-prototypes
    -fno-math-errno -ffreestanding -ffp-contract=off)

include(cmake/rars.cmake)
include(cmake/host.cmake)

add_subdirectory(test)
