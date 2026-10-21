include_guard(GLOBAL)

include(FetchContent)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/3.5.1.tar.gz
    URL_HASH SHA256=5234f4f29473e9a06bc7847d8371858dd135d38466eeeaa652fdc9f8f9ff0c20
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    SYSTEM)
FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.92.9b.tar.gz
    URL_HASH SHA256=21d8a0a565e85dce943e375db00812c2f3f0ab21f3f0f7964e364a63422d7f99
    DOWNLOAD_EXTRACT_TIMESTAMP ON)
FetchContent_Declare(implot
    URL https://github.com/epezent/implot/archive/refs/tags/v1.0.tar.gz
    URL_HASH SHA256=e4a9db64eef7bcc604e2a5ea380af124eb97aa3e8a6a96330079c88add0f7e93
    DOWNLOAD_EXTRACT_TIMESTAMP ON)
FetchContent_MakeAvailable(glfw imgui implot)

find_package(OpenGL REQUIRED)

add_library(zpew_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
add_library(zpew::imgui ALIAS zpew_imgui)
target_include_directories(zpew_imgui SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends)
target_compile_features(zpew_imgui PUBLIC cxx_std_20)
target_compile_definitions(zpew_imgui PRIVATE $<$<PLATFORM_ID:Darwin>:GL_SILENCE_DEPRECATION>)
target_link_libraries(zpew_imgui PUBLIC glfw OpenGL::GL)
zpew_configure_third_party_target(zpew_imgui)

add_library(zpew_implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp)
add_library(zpew::implot ALIAS zpew_implot)
target_include_directories(zpew_implot SYSTEM PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(zpew_implot PUBLIC zpew::imgui)
zpew_configure_third_party_target(zpew_implot)
