#include "platform/glfw_window.hpp"

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <string>

namespace zpew::app {

namespace {

constexpr int kOpenGlMajorVersion = 3;
constexpr int kOpenGlMinorVersion = 3;

std::string last_glfw_error() {
    const char* description = nullptr;
    glfwGetError(&description);
    return description == nullptr ? std::string{"unknown GLFW error"} : std::string{description};
}

void request_core_profile() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kOpenGlMajorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kOpenGlMinorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
}

}

void GlfwWindow::WindowDeleter::operator()(GLFWwindow* window) const noexcept {
    glfwDestroyWindow(window);
}

GlfwWindow::GlfwWindow(const WindowSettings& settings) {
    if (glfwInit() == GLFW_FALSE) {
        throw std::runtime_error{"failed to initialise GLFW: " + last_glfw_error()};
    }

    request_core_profile();
    window_.reset(glfwCreateWindow(settings.width, settings.height, settings.title.c_str(), nullptr,
                                   nullptr));
    if (!window_) {
        const std::string error = last_glfw_error();
        glfwTerminate();
        throw std::runtime_error{"failed to create window: " + error};
    }

    glfwMakeContextCurrent(window_.get());
    glfwSwapInterval(1);
}

GlfwWindow::~GlfwWindow() {
    window_.reset();
    glfwTerminate();
}

bool GlfwWindow::should_close() const {
    return glfwWindowShouldClose(window_.get()) == GLFW_TRUE;
}

void GlfwWindow::clear_framebuffer() const {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_.get(), &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
}

GLFWwindow* GlfwWindow::handle() const noexcept {
    return window_.get();
}

void GlfwWindow::swap_buffers() const {
    glfwSwapBuffers(window_.get());
}

void GlfwWindow::poll_events() {
    glfwPollEvents();
}

}
