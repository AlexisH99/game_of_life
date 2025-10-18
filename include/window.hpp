#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>

class Window {
    public:
        Window(int width, int height, const std::string& title);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

        GLFWwindow* get() const;

        void makeContextCurrent() const;
        void setUserPointer(void* ptr);

    private:

        GLFWwindow* handle = nullptr;

};