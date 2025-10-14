#pragma once
#include "shader.hpp"
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <functional>

class LuaEngine {
    public:
        LuaEngine();
        ~LuaEngine();
        
        void init();
        void registerPrintRedirect(std::function<void(const std::string&)> cb);
        void execute(const std::string& code);
        lua_State* state();

    private:
        lua_State* L;
        std::function<void(const std::string&)> printCB;
};

class Console {
    public:
        Console();
        ~Console();

        void init(GLFWwindow* window);
        void draw(GLFWwindow* window);
        void log(const std::string& s);
        void handleInput(GLFWwindow* win, int key, int action);
        void handleChar(unsigned int codepoint);

        bool visible = false;
        LuaEngine* lua = nullptr;

    private:
        void appendText(std::vector<float>& pts, int x, int y, const std::string& text);

        std::vector<std::string> lines;
        std::string input;
        unsigned int VAO = 0, VBO = 0;
        int fbWidth, fbHeight;
        float cWidth, cHeight;
};