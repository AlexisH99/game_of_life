#pragma once

#include "grid.hpp"
#include "config.hpp"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define IDI_APP_ICON 101

void checkCompileErrors(unsigned int shader, std::string type);

class Application {
    public:
        Application();
        ~Application();

        void run();
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    private:
        void set_window_icon_from_resource(GLFWwindow* window);

        void loadConfig();
        void initGrid();
        int initWindow();
        int initGlad();
        void initShaders();
        void initRender();
        void mainLoop();
        void cleanup();

        Config cfg;
        Grid grid;
        GLFWwindow* window = nullptr;
        
        const char* vertexShaderSource = nullptr;
        const char* fragmentShaderSource = nullptr;

        int fbWidth, fbHeight;

        unsigned int vertexShader = 0;
        unsigned int fragmentShader = 0;
        unsigned int shaderProgram = 0;

        unsigned int VBO = 0, VAO = 0;

        unsigned int textureID = 0;

        std::string title = "GOL";

        bool pause = true;
};