cmake_minimum_required(VERSION 3.14)
project(MY_COMPUTER)

set(CMAKE_CXX_STANDARD 17)

add_executable(MY_COMPUTER main.cpp Super_Stack_Library.h MyLib.h COMPILATOR.h instruction_comp.h)