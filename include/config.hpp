#pragma once

#include <nlohmann/json.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <utility>

using json = nlohmann::json;

class Config {
    public:
        Config();
        ~Config();

        int width = 500;
        int height = 500;
        int gridx = 500;
        int gridy = 500;
        std::string rulestr = "B3S23";
        uint16_t born_rule = 0, survive_rule = 0;
        bool checker = false;
        bool showfps = true;
        bool vsync = false;
        bool freeze_at_start = true;
        
        void initConfig(const std::string& path);
        std::pair<bool, std::string> parseRuleset(std::string rawrulestr);
        void printAllParams() const;

        GLFWwindow* window = nullptr;

    private:
        std::string path;
        void saveConfig(const std::string& path);
        void loadConfig(const std::string& path);
        void printJsonRecursive(const json& j, int indent = 0, const std::string& prefix = "") const;
};