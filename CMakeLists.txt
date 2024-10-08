cmake_minimum_required(VERSION 3.10)

option(KTS_ENABLE_TESTS "build tests" OFF)
option(KTS_ENABLE_MPI "build with MPI support" OFF)

project(SQLiteExample)

if(NOT CMAKE_BUILD_TYPE)
    # Set build type to RelWithDebInfo
    message(STATUS "CMAKE_BUILD_TYPE unset, setting to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)
if (KTS_ENABLE_MPI)
  find_package(MPI REQUIRED)
  message(STATUS "KTS: MPI_VERSION =                 ${MPI_VERSION}")
  message(STATUS "KTS: MPI_CXX_COMPILER =            ${MPI_CXX_COMPILER}")
  message(STATUS "KTS: MPI_CXX_COMPILE_OPTIONS =     ${MPI_CXX_COMPILE_OPTIONS}")
  message(STATUS "KTS: MPI_CXX_COMPILE_DEFINITIONS = ${MPI_CXX_COMPILE_DEFINITIONS}")
  message(STATUS "KTS: MPI_CXX_INCLUDE_DIRS =        ${MPI_CXX_INCLUDE_DIRS}")
  message(STATUS "KTS: MPI_CXX_LINK_FLAGS =          ${MPI_CXX_LINK_FLAGS}")
  message(STATUS "KTS: MPI_CXX_LIBRARIES =           ${MPI_CXX_LIBRARIES}")
endif()

include(CheckFunctionExists)
check_function_exists(getpid KTS_HAVE_GETPID)

if (NOT ${KTS_ENABLE_MPI} AND NOT ${KTS_HAVE_GETPID})
  message(FATAL "Require MPI or getpid() to get a unique process ID")
endif()

add_subdirectory(lib)

add_library(kts SHARED main.cpp kts.cpp kts_pid.cpp)
target_link_libraries(kts PRIVATE kts_schema)
target_link_libraries(kts PRIVATE SQLite::SQLite3)
if (KTS_ENABLE_MPI)
  target_compile_definitions(kts PRIVATE KTS_ENABLE_MPI)
  target_link_libraries(kts PRIVATE MPI::MPI_CXX)
endif()
if (KTS_HAVE_GETPID)
  target_compile_definitions(kts PRIVATE KTS_HAVE_GETPID)
endif()


add_subdirectory(bin)

if(KTS_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(unit_test)
    add_subdirectory(perf_test)
endif()