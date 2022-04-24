cmake_minimum_required(VERSION 3.16)
project(psar CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(lib/json)

add_library(psar_common OBJECT
src/psar_common.cpp
src/psar_common.h
)
target_include_directories(psar_common PUBLIC lib/)
target_link_libraries(psar_common PUBLIC pthread numa)
target_link_libraries(psar_common PRIVATE nlohmann_json::nlohmann_json)

add_executable(test_distant_reads_distant_buffer_forced src/test_distant_reads_distant_buffer_forced.cpp)
target_link_libraries(test_distant_reads_distant_buffer_forced PRIVATE psar_common)

add_executable(test_distant_reads_distant_buffer src/test_distant_reads_distant_buffer.cpp)
target_link_libraries(test_distant_reads_distant_buffer PRIVATE psar_common)

add_executable(test_distant_reads_local_buffer src/test_distant_reads_local_buffer.cpp)
target_link_libraries(test_distant_reads_local_buffer PRIVATE psar_common)

add_executable(test_get_time src/test_get_time.cpp)
target_link_libraries(test_get_time PRIVATE psar_common)

add_executable(test_get_time_all src/test_get_time_all_scenarios.cpp)
target_link_libraries(test_get_time_all PRIVATE psar_common)

add_executable(test_get_time_all_forced src/test_get_time_all_scenarios_forced.cpp)
target_link_libraries(test_get_time_all_forced PRIVATE psar_common)
