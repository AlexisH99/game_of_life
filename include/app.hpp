#pragma once

#include <cstdint>
#include <vector>
#include <random>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "grid.hpp"
#include "config.hpp"


void framebuffer_size_callback([[maybe_unused]]GLFWwindow* window, int width, int height);
void checkCompileErrors(unsigned int shader, std::string type);

class Application {
    public:
        Application();
        ~Application();

        void run();

    private:
        void loadConfig();
        void initGrid();
        int initWindow();
        int initGlad();
        void initShaders();
        void initRender();
        void mainLoop();
        void cleanup();

        void generateRandomTexture(int width, int height);
        void generateCheckerTexture(int width, int height);
        void updateGameOfLife(int width, int height);

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
        std::mt19937 rng;
        std::uniform_int_distribution<int> dist;
        std::vector<uint8_t> current;
        std::vector<uint8_t> next;

        std::string title = "GOL - FPS: ";
};