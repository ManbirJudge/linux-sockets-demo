cmake_minimum_required(VERSION 4.3)
project(LearningSMTP C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(LearningSMTP
    src/main.c
    src/buffer_writer.c
    src/utils.c
)
target_include_directories(LearningSMTP PRIVATE
    include
)