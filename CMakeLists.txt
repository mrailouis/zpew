cmake_minimum_required(VERSION 3.25)

project(zpew
    VERSION 0.1.0
    DESCRIPTION "C++ volatility analytics terminal"
    LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ZPEW_BUILD_TESTS "Build the zpew test suite" ${PROJECT_IS_TOP_LEVEL})
option(ZPEW_BUILD_CLI "Build zpew-cli" ON)
option(ZPEW_ENABLE_SANITIZERS "Build zpew targets with ASan and UBSan where supported" OFF)
option(ZPEW_ENABLE_CLANG_TIDY "Run clang-tidy while compiling zpew targets" OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(ZpewTargetOptions)

add_subdirectory(core)

if(ZPEW_BUILD_CLI)
    add_subdirectory(cli)
endif()

if(ZPEW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
