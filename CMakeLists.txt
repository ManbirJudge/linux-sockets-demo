cmake_minimum_required(VERSION 3.13)
project(LearningSMTP C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(LearningSMTP
    src/main.c
    src/buffer_writer.c
    src/buffer_reader.c
    src/my_string.c
    src/utils.c
)
target_include_directories(LearningSMTP PRIVATE
    include
)
target_compile_options(LearningSMTP PRIVATE
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
target_link_libraries(LearningSMTP PRIVATE m)