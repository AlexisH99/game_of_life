#pragma once

#include <cstdint>
#include <vector>
#include <random>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "grid.hpp"
#include "grid_opt.hpp"
#include "config.hpp"

inline int words_for_width(int N) {
    int minwords = (N + 63) / 64;
    int pad = (minwords * 64) - N;
    return (pad == 0 || pad == 1) ? minwords + 1 : minwords;
}

void checkCompileErrors(unsigned int shader, std::string type);

class Application {
    public:
        Application();
        ~Application();

        void run();
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    private:
        void loadConfig();
        void initGrid();
        int initWindow();
        int initGlad();
        void initShaders();
        void initRender();
        void mainLoop();
        void cleanup();

        Config cfg;
        GridOpt grid;
        GLFWwindow* window = nullptr;
        
        const char* vertexShaderSource = nullptr;
        const char* fragmentShaderSource = nullptr;

        int fbWidth, fbHeight;

        unsigned int vertexShader = 0;
        unsigned int fragmentShader = 0;
        unsigned int shaderProgram = 0;

        unsigned int VBO = 0, VAO = 0;

        unsigned int textureID = 0;

        std::string title = "GOL - FPS: ";

        bool pause = true;
};