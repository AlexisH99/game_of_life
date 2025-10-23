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

        void initConsole();
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

        std::vector<std::string> currentSuggestions;
        int currentSuggestionIndex = -1;
        std::string currentPrefix;

        struct CommandNode {
            std::unordered_map<std::string, CommandNode> children;
            std::function<void(const std::vector<std::string>&)> action;

            CommandNode& add(const std::string& name,
                            std::function<void(const std::vector<std::string>&)> fn = {}) {
                auto& child = children[name];
                child.action = std::move(fn);
                return child;
            }
        };
        CommandNode root;

    private:
        template<typename T>
        std::optional<T> from_string(const std::string& s);

        const CommandNode* findNode(const CommandNode& root, const std::vector<std::string>& tokens);
        void executeCommand(const CommandNode& root, const std::vector<std::string>& tokens);
        std::vector<std::string> suggest(const CommandNode& root, const std::vector<std::string>& tokens, bool endsWithSpace);

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

        std::string input = "";
        std::string suggestionText = "";
        std::vector<std::string> matches;
        std::string prefix;
        bool endsWithSpace = false;
        
        std::vector<float> ptsInput;
        std::vector<float> ptsSuggest;
        std::vector<float> pts;
        
        std::unique_ptr<GLVertexBuffer> vao;
        std::unique_ptr<GLBuffer> vbo;
        std::unique_ptr<GLBuffer> vboLogs;
        std::unique_ptr<GLBuffer> vboInput;
        std::unique_ptr<GLBuffer> vboSuggest;
        std::unique_ptr<GLProgram> shaders;
        
        int fbWidth, fbHeight;

        Config* cfg;
        Window* win;
        Grid* grid;
        Renderer* renderer;
};