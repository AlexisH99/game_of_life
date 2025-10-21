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
#include <deque>

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

        int commandIndex;
        std::deque<std::string> commandHistory;
        bool visible = false;
        bool abortRequested = false;
        float cWidth, cHeight;
        double scrollAccumulator = 0.0f;
        int lineOffset = 0;
        int maxVisibleLines;
        std::deque<std::string> lines;

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
        void setRuleset(std::string rulestr);
        void setSeed(bool isRandom = true, int seed = 0);
        void setDistrib(std::string distType = "uniform", float density = 0.5);
        void getWindowSize();
        void getGridSize();

        std::string input;
        std::vector<float> pts;
        unsigned int VAO = 0, VBO = 0;
        std::unique_ptr<GLVertexBuffer> vao;
        std::unique_ptr<GLBuffer> vbo;
        std::unique_ptr<GLProgram> shaders;
        int fbWidth, fbHeight;

        Config* cfg;
        Window* win;
        Grid* grid;
        Renderer* renderer;
};