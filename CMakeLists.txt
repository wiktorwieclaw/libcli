cmake_minimum_required(VERSION 3.20)
project(libcli)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wconversion -Wpedantic -fconcepts-diagnostics-depth=5)

include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.0.0-preview3
)

FetchContent_MakeAvailable(Catch2)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include/)

add_executable(test test/test.cpp)
target_link_libraries(test PRIVATE ${PROJECT_NAME} Catch2::Catch2WithMain)
