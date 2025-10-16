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

void LuaEngine::execute(const std::string& rawInput) {
    // if (luaL_dostring(L, code.c_str()) != LUA_OK) {
    //     const char* err = lua_tostring(L, -1);
    //     if (printCB) printCB(std::string("[Lua Error] ")+err);
    //     lua_pop(L,1);
    // }
    std::string code;

    // === 1. Tokenize ===
    std::istringstream iss(rawInput);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token)
        tokens.push_back(token);

    if (tokens.empty())
        return;

    // === 2. Commandes spéciales (macros directes) ===
    if (tokens.size() == 1) {
        if (tokens[0] == "stop")  code = "grid.stop()";
        else if (tokens[0] == "start") code = "grid.start()";
        else if (tokens[0] == "reset") code = "grid.reset()";
        else if (tokens[0] == "step") code = "grid.step()";
        // else if (tokens[0] == "save")  code = "grid.save()";
        // else if (tokens[0] == "load")  code = "grid.load()";
    }

    else if (tokens[0] == "step" && tokens.size() >= 2) {
        code = "for i=1," + tokens[1] + " do grid.step() end";
    }

    // === 3. get <object> <property> ===
    else if (tokens[0] == "get" && tokens.size() >= 3) {
        code = "print(" + tokens[1] + "." + tokens[2] + ")";
    }

    // === 4. set <object> <property> <value> ===
    else if (tokens[0] == "set" && tokens.size() >= 4) {
        code = tokens[1] + "." + tokens[2] + " = " + tokens[3];
    }

    // === 5. exec <object> <method> [(args)] ===
    else if (tokens[0] == "exec" && tokens.size() >= 3) {
        // Cas avec arguments entre parenthèses
        std::string args;
        size_t parenPos = rawInput.find('(');
        if (parenPos != std::string::npos) {
            args = rawInput.substr(parenPos); // "(5,10)"
            code = tokens[1] + "." + tokens[2] + args;
        } else {
            code = tokens[1] + "." + tokens[2] + "()";
        }
    }

    // === 6. Sinon, exécute directement du Lua brut ===
    else {
        code = rawInput;
    }

    // === 7. Exécution Lua ===
    if (luaL_dostring(L, code.c_str()) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        if (printCB)
            printCB(std::string("[Lua Error] ") + err);
        lua_pop(L, 1);
    }
}

template<typename T>
void LuaEngine::registerObject(const std::string& name, T* object,
                               const luaL_Reg* methods) {
    lua_pushlightuserdata(L, object);
    luaL_newmetatable(L, name.c_str());
    luaL_setfuncs(L, methods, 0);
    lua_setmetatable(L, -2);
    lua_setglobal(L, name.c_str());
}

lua_State* LuaEngine::state() {
    return L;
}

void LuaEngine::bindConfig(Config* cfg) {
    lua_newtable(L);                  // config
    lua_newtable(L);                  // metatable

    // __index getter
    lua_pushlightuserdata(L, cfg);
    lua_pushcclosure(L, [](lua_State* L)->int {
        Config* cfg = (Config*)lua_touserdata(L, lua_upvalueindex(1));
        std::string key = luaL_checkstring(L, 2);
        if (key == "width")  lua_pushinteger(L, cfg->width);
        else if (key == "height") lua_pushinteger(L, cfg->height);
        else lua_pushnil(L);
        return 1;
    }, 1);
    lua_setfield(L, -2, "__index");

    // __newindex setter
    lua_pushlightuserdata(L, cfg);
    lua_pushcclosure(L, [](lua_State* L)->int {
        Config* cfg = (Config*)lua_touserdata(L, lua_upvalueindex(1));
        std::string key = luaL_checkstring(L, 2);
        if (key == "width") {
            cfg->lua_setWidth(L);
        } else if (key == "height") {
            cfg->lua_setHeight(L);
        }
        return 0;
    }, 1);
    lua_setfield(L, -2, "__newindex");

    lua_setmetatable(L, -2);  // setmetatable(config, mt)
    lua_setglobal(L, "cfg");
}

void LuaEngine::bindGrid(Grid* grid) {
    lua_newtable(L);
    lua_newtable(L);

    lua_pushlightuserdata(L, grid);
    lua_pushcclosure(L, [](lua_State* L)->int {
        Grid* g = (Grid*)lua_touserdata(L, lua_upvalueindex(1));
        std::string key = luaL_checkstring(L, 2);
        if (key == "step") {
            lua_pushlightuserdata(L, g);
            lua_pushcclosure(L, [](lua_State* L)->int {
                Grid* g = (Grid*)lua_touserdata(L, lua_upvalueindex(1));
                g->step();
                return 0;
            }, 1);
            return 1;
        } else if (key == "stop") {
            lua_pushlightuserdata(L, g);
            lua_pushcclosure(L, [](lua_State* L)->int {
                Grid* g = (Grid*)lua_touserdata(L, lua_upvalueindex(1));
                g->stop();
                return 0;
            }, 1);
            return 1;
        } else if (key == "start") {
            lua_pushlightuserdata(L, g);
            lua_pushcclosure(L, [](lua_State* L)->int {
                Grid* g = (Grid*)lua_touserdata(L, lua_upvalueindex(1));
                g->start();
                return 0;
            }, 1);
            return 1;
        } else if (key == "reset") {
            lua_pushlightuserdata(L, g);
            lua_pushcclosure(L, [](lua_State* L)->int {
                Grid* g = (Grid*)lua_touserdata(L, lua_upvalueindex(1));
                g->reset();
                return 0;
            }, 1);
            return 1;
        }
        lua_pushnil(L);
        return 1;
    }, 1);
    lua_setfield(L, -2, "__index");

    lua_setmetatable(L, -2);
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