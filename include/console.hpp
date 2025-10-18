#pragma once

#include "gl_wrappers.hpp"
#include "config.hpp"
#include "window.hpp"
#include "grid.hpp"
#include "renderer.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <memory>

class Console {
    public:
        Console(Config* cfg, Window* win, Grid* grid, Renderer* renderer);
        ~Console();

        void init();
        void draw(GLFWwindow* window);
        void log(const std::string& s);
        void execute(const std::string& command);
        void handleInput(GLFWwindow* win, int key, int action);
        void handleChar(unsigned int codepoint);
        void cleanup();

        bool visible = false;
        bool abortRequested = false;

    private:
        template<typename T>
        std::optional<T> from_string(const std::string& s);
        void appendText(std::vector<float>& pts, int x, int y, const std::string& text);
        void command_start();
        void command_stop();
        void command_regen();
        void command_step(int n_step = 1, float delay = 0.0);
        void setWindowSize(int w, int h);
        void setGridSize(int x, int y);
        void getWindowSize();
        void getGridSize();

        std::vector<std::string> lines;
        std::string input;
        std::vector<float> pts;
        unsigned int VAO = 0, VBO = 0;
        std::unique_ptr<GLVertexBuffer> vao;
        std::unique_ptr<GLBuffer> vbo;
        std::unique_ptr<GLProgram> shaders;
        int fbWidth, fbHeight;
        float cWidth, cHeight;

        Config* cfg;
        Window* win;
        Grid* grid;
        Renderer* renderer;
};