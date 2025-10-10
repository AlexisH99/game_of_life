#pragma once

#include <vector>
#include <random>

inline int w_for_w(int N) {
    int minwords = (N + 63) / 64;
    int pad = (minwords * 64) - N;
    return (pad == 0 || pad == 1) ? minwords + 1 : minwords;
}

class GridOpt {
    public:
        GridOpt();
        ~GridOpt();

        void initSize(int gridx, int gridy, int bs);
        void initMask();
        void initCheckerGrid();
        void initRandomGrid();
        void step();
        void printMask();
        void printCurrent();
        std::vector<uint64_t> getGrid();
        std::vector<uint64_t> getMask();
        const uint32_t* getGrid32Ptr() const;
        int gridX;
        int gridY;
        int leftpad;
        int rows;
        int words_per_row;
        int blocksize;
    private:
        std::mt19937 rng;
        std::uniform_int_distribution<uint64_t> dist;
        std::vector<uint64_t> mask;
        std::vector<uint64_t> current;
        std::vector<uint64_t> next;
};