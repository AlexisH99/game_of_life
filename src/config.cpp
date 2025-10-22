#include "config.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <bitset>

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

    {auto [ok, msg] = parseRuleset(rulestr);
    if (!ok) {
        std::cout << msg << "\n";
        auto [ok, msg] = parseRuleset("B3S23");
    }
    std::cout << msg << "\n";}

    {auto [ok,msg] = parseDistType(distType);
    if (!ok) {
        std::cout << msg << "\n";
        auto [ok, msg] = parseDistType("uniform");
    }
    std::cout << msg << "\n";}
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
        }},
        {"game", {
            {"rulset", rulestr},
            {"random_seed", randomSeed},
            {"seed", seed},
            {"dist_type", distType},
            {"density", density}
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
        if (w.contains("width")) {
            if (w["width"] < 800) {
                width = 800;
            } else {
                width = w["width"];}
            }
        if (w.contains("height")) {
            if (w["height"] < 600) {
                height = 600;
            } else {
                height = w["height"];
            }
        }
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

    if (j.contains("game")) {
        auto& game = j["game"];
        if (game.contains("ruleset"))  rulestr = game["ruleset"];
        if (game.contains("random_seed"))  randomSeed = game["random_seed"];
        if (game.contains("seed"))  seed = game["seed"];
        if (game.contains("dist_type"))  distType = game["dist_type"];
        if (game.contains("density"))  density = game["density"];
    }

    std::cout << "Configuration loaded successfully.\n";
}

std::pair<bool, std::string> Config::parseRuleset(std::string rawrulestr) {
    std::string rulestr;
    std::string errlog;
    born_rule = 0;
    survive_rule = 0;
    for (char c : rawrulestr) {
        if (c != '/' || c != ' ') rulestr.push_back(std::toupper(static_cast<unsigned char>(c)));
    }

    bool in_born = false, in_survive = false;
    bool has_b = false, has_s = false;

    for (int i = 0; i < (int)rulestr.size(); ++i) {
        char c = rulestr[i];

        if (c == 'B') {
            if (has_b) {
                errlog = "[Ruleset Error] multiple 'B' in " + rawrulestr + ". Fallback to default (B3S23).\n";
                return {false, errlog};
            }
            in_born = true; in_survive = false; has_b = true;
            continue;
        }

        if (c == 'S') {
            if (has_s) {
                errlog = "[Ruleset Error] multiple 'B' in " + rawrulestr + ". Fallback to default (B3S23).\n";
                return {false, errlog};
            }
            in_born = false; in_survive = true; has_s = true;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            int n = c - '0';
            if (n < 0 || n > 8) {
                errlog = "[Ruleset Error] out of range value (" + std::to_string(n) + ") in " + rawrulestr + ". Fallback to default (B3S23).\n";
                return {false, errlog};
            }
            if (in_born) born_rule |= (1 << n);
            else if (in_survive) survive_rule |= (1 << n);
            else {
                errlog = "[Ruleset Error] out of range value (" + std::to_string(n) + ") in " + rawrulestr + ". Fallback to default (B3S23).\n";
                return {false, errlog};
            }
        } else {
            errlog = "[Ruleset Error] Invalid '" + std::to_string(c) + "' in " + rawrulestr + ". Fallback to default (B3S23).\n";
            return {false, errlog};
        }
    }

    if (!has_b || !has_s) {
        std::cerr << "[Ruleset Error] Missing B and/or S in " + rawrulestr + ". Fallback to default (B3S23)..\n";
        return {false, errlog};
    }

    errlog = "Loaded ruleset: " + rulestr;

    return {true, errlog};
}

std::pair<bool, std::string> Config::parseDistType(std::string disttyp) {
    if (disttyp == "uniform" || disttyp == "bernoulli") {
        distType = disttyp;
        return {true, "Selected distribution type: " + disttyp};
    } else {
        return {false, "[Dist type Error] Wrong distribution type: " + disttyp + ". Falling back to uniform"};
    }
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
    std::cout << "=========== COMMANDS ===========\n";
    std::cout << "help                      : show help for commands\n";
    std::cout << "stop/start                : pause/unpause simulation\n";
    std::cout << "step                      : do one simulation step\n";
    std::cout << "step <n_steps> <delay>    : do n_steps simulation steps with delay in seconds\n";
    std::cout << "regen                     : regenerate random grid\n";
    std::cout << "set <width> <height>      : set global property (windowSize, gridSize)\n";
    std::cout << "get <globalProperty>      : print current global property (windowSize, gridSize, ruleSet, seed, dist)\n";
    std::cout << "================================\n";
}