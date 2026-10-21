#pragma once

#include "platform/glfw_window.hpp"

namespace zpew::app {

class ImGuiSession {
public:
    explicit ImGuiSession(const GlfwWindow& window);
    ~ImGuiSession();

    ImGuiSession(const ImGuiSession&) = delete;
    ImGuiSession& operator=(const ImGuiSession&) = delete;
    ImGuiSession(ImGuiSession&&) = delete;
    ImGuiSession& operator=(ImGuiSession&&) = delete;

    static void begin_frame();
    static void render(const GlfwWindow& window);
};

}
