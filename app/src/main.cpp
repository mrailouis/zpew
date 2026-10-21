#include "platform/glfw_window.hpp"
#include "platform/imgui_session.hpp"

#include <zpew/version.hpp>

#include <cstdlib>
#include <exception>
#include <imgui.h>
#include <implot.h>
#include <iostream>
#include <string>

namespace {

constexpr int kInitialWidth = 1280;
constexpr int kInitialHeight = 800;

std::string window_title() {
    return "zpew " + std::string{zpew::version()};
}

void draw_main_view() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags kFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoSavedSettings |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("zpew", nullptr, kFlags)) {
        ImGui::TextUnformatted(window_title().c_str());
        if (ImPlot::BeginPlot("##chart", ImVec2{-1.0F, -1.0F})) {
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}

}

int main() {
    try {
        const zpew::app::GlfwWindow window{
            {.width = kInitialWidth, .height = kInitialHeight, .title = window_title()}};
        const zpew::app::ImGuiSession gui{window};

        while (!window.should_close()) {
            zpew::app::GlfwWindow::poll_events();
            zpew::app::ImGuiSession::begin_frame();
            draw_main_view();
            zpew::app::ImGuiSession::render(window);
            window.swap_buffers();
        }
    } catch (const std::exception& error) {
        std::cerr << "zpew-app: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
