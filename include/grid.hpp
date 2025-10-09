#pragma once

#include <vector>
#include <random>

class Grid {
    public:
        Grid();
        ~Grid();

        void initSize(int gridx, int gridy);
        void checkerInit();
        void randomInit();
        void step();
        std::vector<uint8_t> get();
    private:
        std::mt19937 rng;
        std::uniform_int_distribution<int> dist;
        int gridX;
        int gridY;
        std::vector<uint8_t> current;
        std::vector<uint8_t> next;


};