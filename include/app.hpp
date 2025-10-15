#pragma once

#include "config.hpp"
#include "grid.hpp"
#include "window.hpp"
#include "gl_wrappers.hpp"
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

        // Config cfg;
        // Grid grid;
        // Shaders shaders;
        // LuaEngine luaengine;
        // Console console;
        // GLFWwindow* window = nullptr;

        std::unique_ptr<Config> cfg;
        std::unique_ptr<Window> window;
        std::unique_ptr<Grid> grid;
        std::unique_ptr<LuaEngine> luaengine;
        std::unique_ptr<Console> console;

        int fbWidth, fbHeight;

        std::unique_ptr<GLVertexBuffer> vao;
        std::unique_ptr<GLBuffer> vbo;
        std::unique_ptr<GLTexture> texture;
        std::unique_ptr<GLProgram> shaders;

        // GLVertexBuffer vao;
        // GLBuffer vbo{GL_ARRAY_BUFFER};
        // GLTexture texture{GL_TEXTURE};

        std::string title = "GOL";

        bool pause = true;
};