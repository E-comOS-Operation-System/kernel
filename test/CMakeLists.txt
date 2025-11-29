CMake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_C_STANDARD 11)

find_package(cmocka REQUIRED)

include_directories(${CMOCKA_INCLUDE_DIRS})

set(SOURCES test_main.c)

add_executable(test_runner ${SOURCES})
target_link_libraries(test_runner ${CMOCKA_LIBRARIES})