#pragma once

#include <GLFW/glfw3.h>

class Application {
    public:
        void run();

    private:
        void init();
        void mainLoop();
        void cleanup();

        GLFWwindow* window;
};