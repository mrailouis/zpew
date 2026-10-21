cmake_minimum_required(VERSION 3.25)

project(zpew_native LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

set(ZPEW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZPEW_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(ZPEW_BUILD_APP OFF CACHE BOOL "" FORCE)
get_filename_component(zpew_root ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
add_subdirectory(${zpew_root} zpew EXCLUDE_FROM_ALL)

execute_process(
    COMMAND node -p "require('node-addon-api').include_dir"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    OUTPUT_VARIABLE node_addon_api_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
get_filename_component(node_addon_api_dir ${node_addon_api_dir} ABSOLUTE
                       BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

set(zpew_native_build_info ${CMAKE_CURRENT_BINARY_DIR}/generated/build_info.cpp)
add_custom_target(zpew_native_build_info
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${zpew_root}
        -DOUTPUT=${zpew_native_build_info}
        -DNAMESPACE=zpew::native
        "-DCOMPILER=${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
        -P ${zpew_root}/cmake/GenerateBuildInfo.cmake
    BYPRODUCTS ${zpew_native_build_info}
    VERBATIM)

add_library(zpew_native SHARED
    src/addon.cpp
    src/analysis_data.cpp
    src/analysis_object.cpp
    src/asset_list.cpp
    src/chart_data.cpp
    src/js_error.cpp
    src/request_settings.cpp
    src/typed_arrays.cpp
    ${zpew_native_build_info})
add_dependencies(zpew_native zpew_native_build_info)
set_target_properties(zpew_native PROPERTIES PREFIX "" SUFFIX ".node")
target_include_directories(zpew_native PRIVATE src)
target_include_directories(zpew_native SYSTEM PRIVATE ${CMAKE_JS_INC} ${node_addon_api_dir})
target_compile_definitions(zpew_native PRIVATE
    NAPI_VERSION=9
    NAPI_CPP_EXCEPTIONS
    NODE_ADDON_API_DISABLE_DEPRECATED)
target_link_libraries(zpew_native PRIVATE zpew::core ${CMAKE_JS_LIB})
zpew_configure_target(zpew_native)

if(CMAKE_JS_SRC)
    add_library(zpew_native_runtime_hooks OBJECT ${CMAKE_JS_SRC})
    target_include_directories(zpew_native_runtime_hooks SYSTEM PRIVATE ${CMAKE_JS_INC})
    target_link_libraries(zpew_native PRIVATE zpew_native_runtime_hooks)
endif()

if(MSVC AND CMAKE_JS_NODELIB_DEF AND CMAKE_JS_NODELIB_TARGET)
    execute_process(COMMAND ${CMAKE_AR} /def:${CMAKE_JS_NODELIB_DEF} /out:${CMAKE_JS_NODELIB_TARGET}
                    ${CMAKE_STATIC_LINKER_FLAGS})
endif()
