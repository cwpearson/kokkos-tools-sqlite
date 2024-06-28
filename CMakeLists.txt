cmake_minimum_required(VERSION 3.10)

project(SQLiteExample)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SQLite package
find_package(SQLite3 REQUIRED)

# Add the executable
add_library(sqlite_example SHARED main.cpp lib.cpp)

# Link SQLite library
target_link_libraries(sqlite_example PRIVATE SQLite::SQLite3)