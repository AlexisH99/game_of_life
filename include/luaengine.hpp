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
        lua_State* state();

    private:
        template<typename T>
        void addMethod(lua_State* L, const char* name, T* obj, lua_CFunction fn);

        lua_State* L = nullptr;
        std::function<void(const std::string&)> printCB;
};