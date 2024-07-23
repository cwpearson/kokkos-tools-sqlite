add_library(kts_schema STATIC kts_schema.cpp)
target_include_directories(kts_schema INTERFACE ${CMAKE_CURRENT_LIST_DIR})
