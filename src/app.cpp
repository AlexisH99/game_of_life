#include "app.hpp"
#include "config.hpp"

void Application::run() {
    init();
    mainLoop();
    cleanup();
}

void Application::init() {
    glfwInit();
    window = glfwCreateWindow(WIDTH, HEIGHT, "GOL", nullptr, nullptr);
}

void Application::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void Application::cleanup() {
    glfwDestroyWindow(window);
    glfwTerminate();
}