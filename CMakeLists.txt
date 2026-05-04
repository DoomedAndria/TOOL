cmake_minimum_required(VERSION 4.2)
project(untitled C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(untitled
        src/main.c
        src/lib/list/list.c
        src/lib/list/list_types.c
        src/lib/hash/hash.c
        src/lib/string/string.c
        src/lib/vector/vector.c
        src/lib/arena/arena.c
        src/lib/file/file.c
        src/lib/types.c
)
