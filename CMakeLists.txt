cmake_minimum_required(VERSION 3.20)

project(limit_order_book LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(limit_order_book
    src/main.cpp
)