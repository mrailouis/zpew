include(ZpewGuiDependencies)

add_executable(zpew_app
    src/main.cpp
    src/platform/glfw_window.cpp
    src/platform/imgui_session.cpp)

set_target_properties(zpew_app PROPERTIES OUTPUT_NAME zpew-app)
target_include_directories(zpew_app PRIVATE src)
target_compile_definitions(zpew_app PRIVATE $<$<PLATFORM_ID:Darwin>:GL_SILENCE_DEPRECATION>)
target_link_libraries(zpew_app PRIVATE zpew::core zpew::implot)

zpew_configure_target(zpew_app)
