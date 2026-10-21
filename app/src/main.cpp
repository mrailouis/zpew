#include "platform/glfw_window.hpp"
#include "platform/imgui_session.hpp"
#include "ui/terminal_app.hpp"

#include <zpew/data/manifest.hpp>
#include <zpew/version.hpp>

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <implot.h>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

namespace {

constexpr int kInitialWidth = 1600;
constexpr int kInitialHeight = 1000;

std::optional<std::filesystem::path> manifest_from_arguments(std::span<char*> args) {
    if (args.size() == 3 && std::string_view{args[1]} == "--manifest") {
        return std::filesystem::path{args[2]};
    }
    if (const auto from_working_directory =
            zpew::data::find_manifest(std::filesystem::current_path())) {
        return from_working_directory;
    }
    if (!args.empty()) {
        std::error_code error;
        const auto executable = std::filesystem::canonical(args[0], error);
        if (!error) {
            return zpew::data::find_manifest(executable.parent_path());
        }
    }
    return std::nullopt;
}

void configure_plot_style() {
    ImPlotStyle& style = ImPlot::GetStyle();
    style.UseISO8601 = true;
    style.Use24HourClock = true;
    style.UseLocalTime = false;
}

}

int main(int argc, char* argv[]) {
    try {
        const std::span<char*> args{argv, static_cast<std::size_t>(argc)};
        const zpew::app::GlfwWindow window{{.width = kInitialWidth,
                                            .height = kInitialHeight,
                                            .title = "zpew " + std::string{zpew::version()}}};
        const zpew::app::ImGuiSession gui{window};
        configure_plot_style();
        zpew::app::TerminalApp terminal{manifest_from_arguments(args)};

        while (!window.should_close()) {
            zpew::app::GlfwWindow::poll_events();
            zpew::app::ImGuiSession::begin_frame();
            terminal.draw();
            zpew::app::ImGuiSession::render(window);
            window.swap_buffers();
        }
    } catch (const std::exception& error) {
        std::cerr << "zpew-app: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
