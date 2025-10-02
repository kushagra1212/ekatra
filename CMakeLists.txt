cmake_minimum_required(VERSION 3.16)
project(ekatra)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ekatra_lib STATIC
    src/MergeManager.cpp
    src/ProgressBar/ProgressBar.cpp
    src/ProgressReporter/ProgressReporter.cpp
)



target_include_directories(ekatra_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(ekatra
    src/main.cpp
)

if((CMAKE_COMPILER_IS_GNUCC OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # The '-static' flag is a option that instructs the linker to
    # prefer static libraries, bundling libgcc, libstdc++, and libwinpthread
    # directly into the .exe file.
    target_link_options(ekatra PRIVATE -static)
endif()


target_link_libraries(ekatra PRIVATE ekatra_lib)


# --- Testing Setup ---
# Enable testing with CTest
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
 DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(run_tests
  tests/MergeManager_test.cpp 
)

target_link_libraries(run_tests PRIVATE ekatra_lib GTest::gtest GTest::gtest_main)


# Add the test to CTest so it can be run automatically
include(GoogleTest)
gtest_discover_tests(run_tests)