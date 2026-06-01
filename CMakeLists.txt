cmake_minimum_required(VERSION 3.13)
project(LinuxSocketsDemo C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(LinuxSocketsDemo
    src/main.c
    src/buffer_writer.c
    src/buffer_reader.c
    src/my_string.c
    src/hash_table.c
    src/log.c
    src/utils.c
)
target_include_directories(LinuxSocketsDemo PRIVATE
    include
)
target_compile_options(LinuxSocketsDemo PRIVATE
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
target_link_libraries(LinuxSocketsDemo PRIVATE m)