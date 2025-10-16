#include "config.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

Config::Config() {
    
}

Config::~Config() {

}

void Config::initConfig(const std::string& p) {
    path = p;
    if (!std::filesystem::exists(path)) {
        std::cout << "No config.jsonc found. Creating default one..." << std::endl;
        saveConfig(path);
    } else {
        std::cout << "Loading existing config.jsonc..." << std::endl;
        try {
            loadConfig(path);
        } catch (const std::exception& e) {
            std::cerr << "Error reading config: " << e.what() << std::endl;
        }
    }
}

void Config::saveConfig(const std::string& path) {
    json j = {
        {"debug", {
            {"checker", checker},
            {"showfps", showfps}
        }},
        {"display", {
            {"vsync", vsync},
            {"freeze_at_start", freeze_at_start},
        }},
        {"grid", {
            {"gridx", gridx},
            {"gridy", gridy}
        }},
        {"window", {
            {"width", width},
            {"height", height}
        }}
    };

    std::stringstream ss;
    ss << std::setw(4) << j;
    std::string out = ss.str();
    out = R"(// === GAME OF LIFE CONFIG ===
// - debug.checker          : checkerboard grid initialization
// - debug.showfps          : fps counter in window bar
// - display.freezeatstart  : paused simulation at start
// - display.vsync          : vertical synchronization with the screen
// - grid.gridx / gridy     : grid size in horizontal (x) and vertical (y) directions
// - window.width / height    : window size
// Changes needs restart of the application
)" + out;
    std::ofstream ofs(path);
    ofs << out;
    std::cout << "Created default config: " << path << std::endl;
}

// Lecture du JSON existant
void Config::loadConfig(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs)
        throw std::runtime_error("Cannot open config file.");

    std::string line;
    std::stringstream buffer;
    while (std::getline(ifs, line)) {
        if (!line.starts_with("//")) buffer << line << "\n";
    }

    json j = json::parse(buffer, nullptr, true, true);

    if (j.contains("window")) {
        auto& w = j["window"];
        if (w.contains("width"))  width = w["width"];
        if (w.contains("height")) height = w["height"];
    }

    if (j.contains("grid")) {
        auto& g = j["grid"];
        if (g.contains("gridx"))  gridx = g["gridx"];
        if (g.contains("gridy"))  gridy = g["gridy"];
    }

    if (j.contains("debug")) {
        auto& d = j["debug"];
        if (d.contains("checker"))  checker = d["checker"];
        if (d.contains("showfps"))  showfps = d["showfps"];
    }
    
    if (j.contains("display")) {
        auto& disp = j["display"];
        if (disp.contains("vsync"))  vsync = disp["vsync"];
        if (disp.contains("freeze_at_start"))  freeze_at_start = disp["freeze_at_start"];
    }

    std::cout << "Configuration loaded successfully.\n";
}

void Config::printJsonRecursive(const json& j, int indent, const std::string& prefix) const {
    std::string indentation(indent, ' ');

    if (j.is_object()) {
        for (auto it = j.begin(); it != j.end(); ++it) {
            const auto& key = it.key();
            const auto& value = it.value();

            if (value.is_primitive()) {
                std::cout << indentation << prefix << key << " = " << value << "\n";
            } else {
                std::cout << indentation << prefix << key << ":\n";
                printJsonRecursive(value, indent + 4, prefix);
            }
        }
    }
    else if (j.is_array()) {
        int idx = 0;
        for (const auto& elem : j) {
            std::cout << indentation << prefix << "[" << idx++ << "]:\n";
            printJsonRecursive(elem, indent + 4, prefix);
        }
    }
    else if (j.is_primitive()) {
        std::cout << indentation << prefix << j << "\n";
    }
}

void Config::printAllParams() const {
    std::ifstream ifs(path);
    if (!ifs) {
        std::cerr << "Cannot open config file for reading.\n";
        return;
    }

    std::string line;
    std::stringstream buffer;
    while (std::getline(ifs, line)) {
        if (!line.starts_with("//")) buffer << line << "\n";
    }

    json j = json::parse(buffer, nullptr, true, true);

    std::cout << "=== CONFIGURATION PARAMETERS ===\n";
    printJsonRecursive(j, 0);
    std::cout << "========= KEY BINDINGS =========\n";
    std::cout << "Pause/unpause simulation  : Space\n";
    std::cout << "One time step             : Right arrow\n";
    std::cout << "Open/Close console        : F1\n";
    std::cout << "================================\n";
}

int Config::lua_setWidth(lua_State* L) {
    auto cfg = static_cast<Config*>(lua_touserdata(L, lua_upvalueindex(1)));
    cfg->width = (int)luaL_checkinteger(L, 3);
    if (cfg->window) {
        int currentHeight;
        glfwGetWindowSize(cfg->window, nullptr, &currentHeight);
        glfwSetWindowSize(cfg->window, cfg->width, currentHeight);
    }
    return 0;
}

int Config::lua_setHeight(lua_State* L) {
    auto cfg = static_cast<Config*>(lua_touserdata(L, lua_upvalueindex(1)));
    cfg->height = (int)luaL_checkinteger(L, 3);
    if (cfg->window) {
        int currentWidth;
        glfwGetWindowSize(cfg->window, &currentWidth, nullptr);
        glfwSetWindowSize(cfg->window, currentWidth, cfg->height);
    }
    return 0;
}