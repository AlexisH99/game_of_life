#pragma once
#include "gl_wrappers.hpp"
#include "config.hpp"
#include "grid.hpp"
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
#include <memory>

class LuaEngine {
    public:
        LuaEngine();
        ~LuaEngine();
        
        void init();
        void registerPrintRedirect(std::function<void(const std::string&)> cb);
        void execute(const std::string& rawInput);
        void bindConfig(Config* cfg);
        void bindGrid(Grid* grid);
        template<typename T>
        void registerObject(const std::string& name, T* object,
            const luaL_Reg* methods);
        lua_State* state();

    private:
        lua_State* L = nullptr;
        std::function<void(const std::string&)> printCB;
};

class Console {
    public:
        Console();
        ~Console();

        void init();
        void draw(GLFWwindow* window);
        void log(const std::string& s);
        void handleInput(GLFWwindow* win, int key, int action);
        void handleChar(unsigned int codepoint);
        void cleanup();

        bool visible = false;
        LuaEngine* lua = nullptr;

    private:
        void appendText(std::vector<float>& pts, int x, int y, const std::string& text);

        std::vector<std::string> lines;
        std::string input;
        std::vector<float> pts;
        unsigned int VAO = 0, VBO = 0;
        std::unique_ptr<GLVertexBuffer> vao;
        std::unique_ptr<GLBuffer> vbo;
        std::unique_ptr<GLProgram> shaders;
        int fbWidth, fbHeight;
        float cWidth, cHeight;
};