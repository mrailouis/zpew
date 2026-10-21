#pragma once

#include <memory>
#include <string>

struct GLFWwindow;

namespace zpew::app {

struct WindowSettings {
    int width;
    int height;
    std::string title;
};

class GlfwWindow {
public:
    explicit GlfwWindow(const WindowSettings& settings);
    ~GlfwWindow();

    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;
    GlfwWindow(GlfwWindow&&) = delete;
    GlfwWindow& operator=(GlfwWindow&&) = delete;

    [[nodiscard]] bool should_close() const;
    [[nodiscard]] GLFWwindow* handle() const noexcept;
    void clear_framebuffer() const;
    void swap_buffers() const;

    static void poll_events();

private:
    struct WindowDeleter {
        void operator()(GLFWwindow* window) const noexcept;
    };

    std::unique_ptr<GLFWwindow, WindowDeleter> window_;
};

}
