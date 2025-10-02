cmake_minimum_required(VERSION 3.16)
project(ekatra)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(ekatra 
    src/MergeManager.cpp
    src/main.cpp
)

if(MSVC)
  # This tells the MSVC compiler to use the static C++ runtime library (/MT)
  # instead of the dynamic one (/MD).
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# endif()

# if((CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
#     # The '-static' flag is a option that instructs the linker to
#     # prefer static libraries, bundling libgcc, libstdc++, and libwinpthread
#     # directly into the .exe file.
#     target_link_options(ekatra PRIVATE -static)
# endif()


target_include_directories(ekatra PUBLIC include)

# --- Testing Setup ---
# Enable testing with CTest
enable_testing()

# Fetch GoogleTest from repository
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
 DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

# Create a new executable for our tests
add_executable(run_tests
  tests/MergeManager_test.cpp # Path to your new test file
  src/MergeManager.cpp # Include the source file you are testing
)

# Link the test executable against GoogleTest and your application's include paths
target_link_libraries(run_tests PRIVATE  GTest::gtest GTest::gtest_main)
target_include_directories(run_tests PRIVATE include)



# Add the test to CTest so it can be run automatically
include(GoogleTest)
gtest_discover_tests(run_tests)