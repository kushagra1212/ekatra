cmake_minimum_required(VERSION 3.16)
project(ekatra)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Since argparse.hpp is in the include directory
include_directories(include)

add_executable(ekatra 
  main.cpp 
  MergeManager.cpp 
  MergeManager.h
)