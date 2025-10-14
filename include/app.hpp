#pragma once

#include "grid.hpp"
#include "shader.hpp"
#include "config.hpp"
#include "console.hpp"
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

        Shaders shaders;

    private:
        void set_window_icon_from_resource(GLFWwindow* window);

        void loadConfig();
        void initGrid();
        int initWindow();
        int initGlad();
        void initRender();
        void initConsole();
        void mainLoop();
        void cleanup();

        Config cfg;
        Grid grid;
        LuaEngine luaengine;
        Console console;
        GLFWwindow* window = nullptr;

        int fbWidth, fbHeight;

        unsigned int VBO = 0, VAO = 0;

        unsigned int textureID = 0;

        std::string title = "GOL";

        bool pause = true;
};