#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

#define WIDTH 800
#define HEIGHT 600
#define GRIDX 500
#define GRIDY 500

using json = nlohmann::json;

class Config {
    public:
        Config();
        Config(const std::string& p);
        ~Config();

        int width = 500;
        int height = 500;
        int gridx = 500;
        int gridy = 500;

        void printAllParams() const;

    private:
        std::string path;
        void saveConfig(const std::string& path);
        void loadConfig(const std::string& path);
        void printJsonRecursive(const json& j, int indent = 0, const std::string& prefix = "") const;
};