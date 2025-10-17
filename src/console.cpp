#include "console.hpp"
#include "shaders_sources.hpp"
#include "font8x8_basic.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>

LuaEngine::LuaEngine() {

}
    
LuaEngine::~LuaEngine() {
    if (L) lua_close(L);
}

void LuaEngine::init() {
    std::cout << "Lua version: " << LUA_VERSION << std::endl;
    if (L) lua_close(L);
    L = luaL_newstate();
    luaL_openlibs(L);
}

void LuaEngine::registerPrintRedirect(std::function<void(const std::string&)> cb) {
    printCB = cb;
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, [](lua_State* L)->int {
        LuaEngine* self = (LuaEngine*)lua_touserdata(L, lua_upvalueindex(1));
        std::string out;
        int n = lua_gettop(L);
        for (int i=1;i<=n;i++){
            size_t len;
            const char* s = luaL_tolstring(L, i, &len);
            out += s;
            if (i<n) out += '\t';
            lua_pop(L,1);
        }
        if (self->printCB) self->printCB(out);
        else std::cout << out << "\n";
        return 0;
    }, 1);
    lua_setglobal(L, "print");
}

lua_State* LuaEngine::state() {
    return L;
}

void LuaEngine::execute(const std::string& rawInput) {
    std::string code = "print('> "+ rawInput + "');";

    // === 1. Tokenize ===
    std::istringstream iss(rawInput);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token)
        tokens.push_back(token);

    if (tokens.empty())
        return;

    if (tokens.size() == 1) {
        if (tokens[0] == "stop")  code += "grid.stop()";
        else if (tokens[0] == "start") code += "grid.start()";
        else if (tokens[0] == "reset") code += "grid.reset()";
        else if (tokens[0] == "step") code += "grid.step()";
        // else if (tokens[0] == "save")  code = "grid.save()";
        // else if (tokens[0] == "load")  code = "grid.load()";
    }

    else if (tokens[0] == "step" && tokens.size() >= 2) {
        code += "for i=1," + tokens[1] + " do grid:step() end";
    }
    
    else if (tokens[0] == "resize" && tokens.size() == 3) {
        if (tokens[1] == "." && tokens[2] == ".") {
            code += "";
        } else if (tokens[1] == ".") {
            code += "cfg.resize(cfg.width," + tokens[2] + ");";
        } else if (tokens[2] == ".") {
            code += "cfg.resize(" + tokens[1] + ",cfg.height);";
        } else {
            code += "cfg.resize(" + tokens[1] + "," + tokens[2] + ");";
        }
        code += "print('window resized to ' .. cfg.width .. 'x' .. cfg.height)";
    }

    else if (tokens[0] == "get" && tokens.size() == 2) {
        if (tokens[1] == "windowSize") code += "local w, h = cfg.windowSize(); print('" + tokens[1] + ": ' .. w .. 'x' .. h)";
        if (tokens[1] == "gridSize") code += "local w, h = cfg.gridSize(); print('" + tokens[1] + ": ' .. w .. 'x' .. h)";
    }

    else {
        code += rawInput;
    }

    if (luaL_dostring(L, code.c_str()) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        if (printCB)
            printCB(std::string("[Lua Error] ") + err);
        lua_pop(L, 1);
    }
}

template<typename T>
void LuaEngine::addMethod(lua_State* L, const char* name, T* obj, lua_CFunction fn) {
    lua_pushlightuserdata(L, obj);
    lua_pushcclosure(L, fn, 1);
    lua_setfield(L, -2, name);
}

void LuaEngine::bindConfig(Config* cfg) {
    lua_newtable(L);

    addMethod(L, "resize", cfg, [](lua_State* L) -> int {
        auto* cfg = (Config*)lua_touserdata(L, lua_upvalueindex(1));
        int w = (int)luaL_checkinteger(L, 1);
        int h = (int)luaL_checkinteger(L, 2);
        cfg->width = w;
        cfg->height = h;
        if (cfg->window) glfwSetWindowSize(cfg->window, w, h);
        return 0;
    });

    addMethod(L, "windowSize", cfg, [](lua_State* L) -> int {
        auto* cfg = (Config*)lua_touserdata(L, lua_upvalueindex(1));
        lua_pushinteger(L, cfg->width);
        lua_pushinteger(L, cfg->height);
        return 2;
    });

    addMethod(L, "gridSize", cfg, [](lua_State* L) -> int {
        auto* cfg = (Config*)lua_touserdata(L, lua_upvalueindex(1));
        lua_pushinteger(L, cfg->gridx);
        lua_pushinteger(L, cfg->gridy);
        return 2;
    });

    addMethod(L, "getWidth", cfg, [](lua_State* L) -> int {
        auto* cfg = (Config*)lua_touserdata(L, lua_upvalueindex(1));
        lua_pushinteger(L, cfg->width);
        return 1;
    });

    addMethod(L, "getHeight", cfg, [](lua_State* L) -> int {
        auto* cfg = (Config*)lua_touserdata(L, lua_upvalueindex(1));
        lua_pushinteger(L, cfg->height);
        return 1;
    });

    lua_newtable(L);
    lua_pushlightuserdata(L, cfg);
    lua_pushcclosure(L, [](lua_State* L) -> int {
        auto* cfg = (Config*)lua_touserdata(L, lua_upvalueindex(1));
        std::string key = luaL_checkstring(L, 2);
        if (key == "width") lua_pushinteger(L, cfg->width);
        else if (key == "height") lua_pushinteger(L, cfg->height);
        else lua_pushnil(L);
        return 1;
    }, 1);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    lua_setglobal(L, "cfg");
}

void LuaEngine::bindGrid(Grid* g) {
    lua_newtable(L);

    addMethod(L, "start", g, [](lua_State* L) -> int {
        ((Grid*)lua_touserdata(L, lua_upvalueindex(1)))->start();
        return 0;
    });

    addMethod(L, "stop", g, [](lua_State* L) -> int {
        ((Grid*)lua_touserdata(L, lua_upvalueindex(1)))->stop();
        return 0;
    });

    addMethod(L, "reset", g, [](lua_State* L) -> int {
        ((Grid*)lua_touserdata(L, lua_upvalueindex(1)))->reset();
        return 0;
    });

    addMethod(L, "step", g, [](lua_State* L) -> int {
        ((Grid*)lua_touserdata(L, lua_upvalueindex(1)))->step();
        return 0;
    });

    lua_setglobal(L, "grid");
}

Console::Console() {

}

Console::~Console() {

}

void Console::init() {
    vao = std::make_unique<GLVertexBuffer>();
    vbo = std::make_unique<GLBuffer>(GL_ARRAY_BUFFER);
    shaders = std::make_unique<GLProgram>(consoleVert, consoleFrag);
}

void Console::log(const std::string& s) {
    lines.push_back(s);
    if (lines.size() > 100) lines.erase(lines.begin());
}

void Console::draw(GLFWwindow* window) {
    if (!visible) return;
    shaders->use();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    cWidth = (float)fbWidth;
    cHeight = (float)(0.3 * fbHeight);

    // === fond semi-transparent ===
    float bgVerts[12] = {
        0, 0,  cWidth, 0,  cWidth, cHeight,
        0, 0,  cWidth, cHeight, 0, cHeight
    };
    vao->bind();
    vbo->bind();
    vbo->set_data(sizeof(bgVerts), bgVerts, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glUniform2f(glGetUniformLocation(shaders->get(), "uScreen"), (float)fbWidth, (float)fbHeight);
    glUniform4f(glGetUniformLocation(shaders->get(), "uColor"), 0.0f, 0.0f, 0.15f, 0.75f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // === texte vert ===
    pts.clear();
    int lineHeight = 10;
    int maxVisibleLines = (int)(cHeight / lineHeight) - 3;
    if (maxVisibleLines < 1) maxVisibleLines = 1;

    int y = 10;
    for (int i = std::max(0, (int)lines.size() - maxVisibleLines); i < (int)lines.size(); i++) {
        appendText(pts, 10, y, lines[i]);
        y += lineHeight;
    }
    appendText(pts, 10, cHeight - lineHeight, "> " + input + "_");

    vbo->set_data(pts.size() * sizeof(float), pts.data(), GL_STREAM_DRAW);
    glUniform4f(glGetUniformLocation(shaders->get(), "uColor"), 0.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_POINTS, 0, pts.size()/2);

    glDisable(GL_BLEND);
}

void Console::handleInput([[maybe_unused]]GLFWwindow* win, int key, int action) {
    if (!visible) return;
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_ENTER) {
            lua->execute(input);
            input.clear();
        } else if (key == GLFW_KEY_BACKSPACE && !input.empty()) {
            input.pop_back();
        }
    }
}

void Console::handleChar(unsigned int codepoint) {
    if (!visible) return;
    if (codepoint >= 32 && codepoint < 127)
        input.push_back((char)codepoint);
}

void Console::appendText(std::vector<float>& pts, int x, int y, const std::string& text) {
    for (size_t i = 0; i < text.size(); i++) {
        unsigned char c = text[i];
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (font8x8_basic[c][row] & (1 << col)) {
                    pts.push_back((float)(x + col + i*8));
                    pts.push_back((float)(y + row));
                }
            }
        }
    }
}

void Console::cleanup() {

}