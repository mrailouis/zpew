add_executable(zpew_cli
    src/check_data_command.cpp
    src/forecast_command.cpp
    src/main.cpp
    src/manifest_path.cpp
    src/options.cpp)

set_target_properties(zpew_cli PROPERTIES OUTPUT_NAME zpew-cli)
target_link_libraries(zpew_cli PRIVATE zpew::core)

zpew_configure_target(zpew_cli)
