#pragma once

#include "config.hpp"
#include "grid.hpp"
#include "window.hpp"
#include "console.hpp"
#include "renderer.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define IDI_APP_ICON 101

class Application {
    public:
        Application();
        ~Application();

        void run();
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
        static void char_callback(GLFWwindow* window, unsigned int codepoint);
        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
        static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

    private:
        void set_window_icon_from_resource(GLFWwindow* window);

        void loadConfig();
        void initGrid();
        void initWindow();
        void initGlad();
        void initRender();
        void initConsole();
        void mainLoop();

        std::unique_ptr<Config> cfg;
        std::unique_ptr<Window> window;
        std::unique_ptr<Grid> grid;
        std::unique_ptr<Console> console;
        std::unique_ptr<Renderer> renderer;

        int fbWidth, fbHeight;

        std::string title = "GOL";
};