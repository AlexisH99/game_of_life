#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

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
        
        void initConfig(const std::string& path);
        void printAllParams() const;

    private:
        std::string path;
        void saveConfig(const std::string& path);
        void loadConfig(const std::string& path);
        void printJsonRecursive(const json& j, int indent = 0, const std::string& prefix = "") const;
};