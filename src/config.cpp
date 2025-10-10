#include "config.hpp"

Config::Config() {
    
}

Config::~Config() {

}

void Config::initConfig(const std::string& p) {
    path = p;
    if (!std::filesystem::exists(path)) {
        std::cout << "No config.json found. Creating default one..." << std::endl;
        saveConfig(path);
    } else {
        std::cout << "Loading existing config.json..." << std::endl;
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
            {"checker", checker}
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

    std::ofstream ofs(path);
    ofs << std::setw(4) << j << std::endl;
    std::cout << "Created default config: " << path << std::endl;
}

// Lecture du JSON existant
void Config::loadConfig(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs)
        throw std::runtime_error("Cannot open config file.");

    json j;
    ifs >> j;

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

    json j;
    ifs >> j;
    std::cout << "=== CONFIGURATION PARAMETERS ===\n";
    printJsonRecursive(j, 0);
    std::cout << "================================\n";
}