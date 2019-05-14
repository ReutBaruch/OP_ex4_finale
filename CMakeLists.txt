cmake_minimum_required(VERSION 3.14)
project(ex4_2 C)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(ex4_2 main.c osqueue.h osqueue.c threadPool.h threadPool.c)