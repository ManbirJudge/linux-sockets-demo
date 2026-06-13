cmake_minimum_required(VERSION 3.13)
project(RDR C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(RDR
    src/main.c
    src/buffer_writer.c
    src/buffer_reader.c
    src/my_string.c
    src/hash_table.c
    src/log.c
    src/utils.c
)
target_include_directories(RDR PRIVATE
    include
)
target_compile_options(RDR PRIVATE
    $<$<CONFIG:Debug>:
        -g
        -O0
        -Wall
        -Wextra
    >
    $<$<CONFIG:Release>:
        -O2
        -march=native
        -Wall
        -Wextra
    >
)
target_link_libraries(RDR PRIVATE m)
set_target_properties(RDR PROPERTIES
    OUTPUT_NAME "rdr"
)
