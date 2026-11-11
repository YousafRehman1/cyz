cmake_minimum_required(VERSION 3.10)
project(mysh C)
set(CMAKE_C_STANDARD 11)
add_executable(mysh main.c parser.c exec.c builtins.c)
target_compile_options(mysh PRIVATE -Wall -Wextra -g)
# After adding the official library files, enable with:
# cmake -S . -B build -DUSE_LINENOISE=ON
option(USE_LINENOISE "Build with official linenoise library" OFF)
if(USE_LINENOISE)
    target_sources(mysh PRIVATE linenoise.c)
    target_compile_definitions(mysh PRIVATE USE_LINENOISE)
endif()
