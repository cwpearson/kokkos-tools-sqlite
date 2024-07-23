include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip
)
message(STATUS "Retrieving remote gh/nlohmann/json")
FetchContent_MakeAvailable(nlohmann_json)

add_executable(chrome-tracing chrome-tracing.cpp)
target_link_libraries(chrome-tracing PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(chrome-tracing PRIVATE SQLite::SQLite3)
target_link_libraries(chrome-tracing PRIVATE kts_schema)
