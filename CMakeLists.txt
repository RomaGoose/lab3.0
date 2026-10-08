cmake_minimum_required(VERSION 3.14)
project(lab2 CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(tests
    tests/uniq_test.cpp
)

target_compile_options(tests PRIVATE -fsanitize=address,undefined)
target_link_options(tests PRIVATE -fsanitize=address,undefined)

target_include_directories(tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(tests PRIVATE
    Catch2::Catch2WithMain
)


enable_testing()

include(Catch)
catch_discover_tests(tests)
