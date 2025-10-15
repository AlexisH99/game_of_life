#include "window.hpp"
#include <iostream>
#include <cstdlib>

Window::Window(int width, int height, const std::string& title) {
    if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    handle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    makeContextCurrent();
    setUserPointer(this);
}

Window::~Window() {
    if (handle) {
        glfwDestroyWindow(handle);
        glfwTerminate();
    }
}

GLFWwindow* Window::get() const {
    return handle;
}

void Window::makeContextCurrent() const {
    glfwMakeContextCurrent(handle);
}

void Window::setUserPointer(void* ptr) {
    glfwSetWindowUserPointer(handle, ptr);
}

Window::Window(Window&& other) noexcept : handle(other.handle) {
    other.handle = nullptr;
}

Window& Window::operator=(Window&& other) noexcept {
    std::swap(handle, other.handle);
    return *this;
}