#pragma once

#include "config.hpp"

#include <vector>
#include <random>

inline int w_for_w(int N) {
    int minwords = (N + 63) / 64;
    int pad = (minwords * 64) - N;
    return (pad == 0 || pad == 1) ? minwords + 1 : minwords;
}

class Grid {
    public:
        Grid();
        ~Grid();

        void initSize();
        void initRuleset();
        void initMask();

        void initCheckerGrid();
        void initRandomGrid();

        void reset();
        void step();
        void start();
        void stop();
        
        void printMask();
        void printCurrent();

        std::vector<uint64_t> getGrid();
        std::vector<uint64_t> getMask();
        const uint32_t* getGrid32Ptr() const;

        int leftpad;
        int rows;
        int words_per_row;
        int blocksize;
        bool pause = true;

        Config* cfg = nullptr;
    private:
        std::mt19937 rng;
        std::uniform_int_distribution<uint64_t> dist;
        std::vector<uint64_t> mask;
        std::vector<uint64_t> current;
        std::vector<uint64_t> next;
        std::vector<uint64_t> count;

        uint16_t born_rule = 0b0000000000001000;
        uint16_t survive_rule = 0b0000000000001100;
};