cmake_minimum_required(VERSION 3.10)

if (KTS_EXTERNAL_KOKKOS)
  find_package(Kokkos REQUIRED)
else()
  include(FetchContent)
  message(STATUS "acquiring remote Kokkos...")
  FetchContent_Declare(Kokkos
    GIT_REPOSITORY    git@github.com:kokkos/kokkos.git
    GIT_TAG           master
  )
  FetchContent_MakeAvailable(Kokkos)
endif()

function (kts_add_test tgt)
  add_executable(${tgt} ${ARGN})
  target_link_libraries(${tgt} Kokkos::kokkos)
  add_test(NAME ${tgt} COMMAND ${tgt})
  set_property(TEST ${tgt} PROPERTY ENVIRONMENT "KOKKOS_TOOLS_LIBS=${CMAKE_BINARY_DIR}/libkts.so")
endfunction()

kts_add_test(test_parfor test_parfor.cpp)
kts_add_test(test_deep_copy test_deep_copy.cpp)
kts_add_test(test_view_init test_view_init.cpp)


