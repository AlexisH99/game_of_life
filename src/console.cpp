#include "console.hpp"
#include "app.hpp"
#include "font8x8_basic.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

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

void LuaEngine::execute(const std::string& code) {
    if (luaL_dostring(L, code.c_str()) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        if (printCB) printCB(std::string("[Lua Error] ")+err);
        lua_pop(L,1);
    }
}

lua_State* LuaEngine::state() {
    return L;
}

Console::Console() {

}

Console::~Console() {

}

void Console::init() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

void Console::log(const std::string& s) {
    lines.push_back(s);
    if (lines.size() > 100) lines.erase(lines.begin());
}

void Console::draw(GLFWwindow* window) {
    if (!visible) return;
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    glUseProgram(app->shaders.consoleShader.program);
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
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bgVerts), bgVerts, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glUniform2f(glGetUniformLocation(app->shaders.consoleShader.program, "uScreen"), (float)fbWidth, (float)fbHeight);
    glUniform4f(glGetUniformLocation(app->shaders.consoleShader.program, "uColor"), 0.0f, 0.0f, 0.15f, 0.75f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // === texte vert ===
    std::vector<float> pts;
    int lineHeight = 10;
    int maxVisibleLines = (int)(cHeight / lineHeight) - 3;
    if (maxVisibleLines < 1) maxVisibleLines = 1;

    int y = 10;
    for (int i = std::max(0, (int)lines.size() - maxVisibleLines); i < (int)lines.size(); i++) {
        appendText(pts, 10, y, lines[i]);
        y += lineHeight;
    }
    appendText(pts, 10, cHeight - lineHeight, "> " + input + "_");

    glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(float), pts.data(), GL_STREAM_DRAW);
    glUniform4f(glGetUniformLocation(app->shaders.consoleShader.program, "uColor"), 0.0f, 1.0f, 0.0f, 1.0f);
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
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}