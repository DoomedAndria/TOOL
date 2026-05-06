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
        src/compiler/lexer/lexer.h
        src/compiler/lexer/lexer.c
        src/compiler/ast/ast.h
        src/compiler/ast/ast.c
        src/compiler/parser/parser.h
        src/compiler/parser/parser.c
)
