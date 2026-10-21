include(ZpewCliDependencies)

set(zpew_build_info_source ${CMAKE_CURRENT_BINARY_DIR}/generated/build_info.cpp)
add_custom_target(zpew_build_info
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
        -DOUTPUT=${zpew_build_info_source}
        "-DCOMPILER=${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
        -P ${PROJECT_SOURCE_DIR}/cmake/GenerateBuildInfo.cmake
    BYPRODUCTS ${zpew_build_info_source}
    COMMENT "Recording git commit for zpew-cli"
    VERBATIM)

add_executable(zpew_cli
    src/check_data_command.cpp
    src/eval_command.cpp
    src/eval_config.cpp
    src/fit_command.cpp
    src/forecast_command.cpp
    src/json_text.cpp
    src/main.cpp
    src/manifest_path.cpp
    src/model_factory.cpp
    src/options.cpp
    src/results_writer.cpp
    ${zpew_build_info_source})
add_dependencies(zpew_cli zpew_build_info)

set_target_properties(zpew_cli PROPERTIES OUTPUT_NAME zpew-cli)
target_include_directories(zpew_cli PRIVATE src)
target_link_libraries(zpew_cli PRIVATE zpew::core tomlplusplus::tomlplusplus)

zpew_configure_target(zpew_cli)
