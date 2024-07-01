cmake_minimum_required(VERSION 3.10)

option(KTS_ENABLE_TESTS "build tests" OFF)

project(SQLiteExample)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SQLite package
find_package(SQLite3 REQUIRED)

# Add the executable
add_library(kts SHARED main.cpp kts.cpp)

# Link SQLite library
target_link_libraries(kts PRIVATE SQLite::SQLite3)

if(KTS_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(unit_test)
    add_subdirectory(perf_test)
endif()