add_library(zpew_core STATIC
    src/io/binary_file.cpp
    src/io/sha256.cpp
    src/version.cpp)
add_library(zpew::core ALIAS zpew_core)

target_include_directories(zpew_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(zpew_core PUBLIC cxx_std_20)
target_compile_definitions(zpew_core PRIVATE ZPEW_VERSION="${PROJECT_VERSION}")

zpew_configure_target(zpew_core)
