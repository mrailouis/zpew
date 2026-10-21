include(ZpewGuiDependencies)

add_executable(zpew_app
    src/chart/candlestick_plot.cpp
    src/chart/chart_series.cpp
    src/chart/chart_view.cpp
    src/main.cpp
    src/platform/glfw_window.cpp
    src/platform/imgui_session.cpp
    src/ui/symbol_picker.cpp
    src/ui/terminal_app.cpp)

set_target_properties(zpew_app PROPERTIES OUTPUT_NAME zpew-app)
target_include_directories(zpew_app PRIVATE src)
target_compile_definitions(zpew_app PRIVATE $<$<PLATFORM_ID:Darwin>:GL_SILENCE_DEPRECATION>)
target_link_libraries(zpew_app PRIVATE zpew::core zpew::implot)

if(APPLE)
    set(zpew_app_icon ${CMAKE_CURRENT_SOURCE_DIR}/resources/zpew.icns)
    target_sources(zpew_app PRIVATE ${zpew_app_icon})
    set_source_files_properties(${zpew_app_icon} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(zpew_app PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist.in
        MACOSX_BUNDLE_BUNDLE_NAME zpew
        MACOSX_BUNDLE_GUI_IDENTIFIER net.zpew.app
        MACOSX_BUNDLE_ICON_FILE zpew.icns
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
elseif(WIN32)
    target_sources(zpew_app PRIVATE resources/zpew.rc)
endif()

zpew_configure_target(zpew_app)
