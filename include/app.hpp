#pragma once

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#include <glad/glad.h>
#include <GLFW/glfw3.h>


void framebuffer_size_callback([[maybe_unused]]GLFWwindow* window, int width, int height);
void checkCompileErrors(unsigned int shader, std::string type);

class Application {
    public:
        void run();

    private:
        int initWindow();
        int initGlad();
        void initShaders();
        void initRender();
        void mainLoop();
        void cleanup();

        void generateTextureData(int width, int height);
        void generateCheckerTexture(int width, int height);

        GLFWwindow* window = nullptr;
        const char* vertexShaderSource = nullptr;
        const char* fragmentShaderSource = nullptr;

        unsigned int vertexShader = 0;
        unsigned int fragmentShader = 0;
        unsigned int shaderProgram = 0;

        unsigned int VBO = 0, VAO = 0;

        unsigned int textureID = 0;
        std::vector<unsigned char> pixels;
};