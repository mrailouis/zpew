#include "platform/imgui_session.hpp"

#include "platform/glfw_window.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <stdexcept>

namespace zpew::app {

namespace {

constexpr const char* kGlslVersion = "#version 330";

}

ImGuiSession::ImGuiSession(const GlfwWindow& window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window.handle(), true)) {
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        throw std::runtime_error{"failed to initialise the ImGui GLFW backend"};
    }
    if (!ImGui_ImplOpenGL3_Init(kGlslVersion)) {
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        throw std::runtime_error{"failed to initialise the ImGui OpenGL 3 backend"};
    }
}

ImGuiSession::~ImGuiSession() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void ImGuiSession::begin_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiSession::render(const GlfwWindow& window) {
    ImGui::Render();
    window.clear_framebuffer();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}
