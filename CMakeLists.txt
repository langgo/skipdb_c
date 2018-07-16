cmake_minimum_required(VERSION 3.10)
project(skipdb C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(skipdb
        main.c
        skip_node.c
        skip_node.h
        skipdb.c
        skipdb.h)
