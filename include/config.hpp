#pragma once

#include <nlohmann/json.hpp>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <glad/gl.h>
#include <GLFW/glfw3.h>

using json = nlohmann::json;

class Config {
    public:
        Config();
        ~Config();

        int width = 500;
        int height = 500;
        int gridx = 500;
        int gridy = 500;
        bool checker = false;
        bool showfps = true;
        bool vsync = false;
        bool freeze_at_start = true;
        
        void initConfig(const std::string& path);
        void printAllParams() const;

        static int lua_getWidth(lua_State* L);
        static int lua_getHeight(lua_State* L);
        static int lua_setWidth(lua_State* L);
        static int lua_setHeight(lua_State* L);

        GLFWwindow* window = nullptr;

    private:
        std::string path;
        void saveConfig(const std::string& path);
        void loadConfig(const std::string& path);
        void printJsonRecursive(const json& j, int indent = 0, const std::string& prefix = "") const;
};