#include <grid.hpp>
#include <iostream>
#include <cstdlib>

Grid::Grid() : rng(std::random_device{}()), dist(0, ~0ULL) {

}

Grid::~Grid() {

}

void Grid::initSize(int gridx, int gridy, int bs) {
    gridX = gridx;
    gridY = gridy;
    rows = gridy + 2;
    words_per_row = w_for_w(gridx);
    blocksize = bs;
    mask.resize(rows * words_per_row);
    current.resize(rows * words_per_row);
    next.resize(rows * words_per_row);
}

void Grid::initMask() {
    int pad = ((words_per_row * 64) - gridX);
    leftpad = pad / 2;
    int rightpad = leftpad + pad % 2;
    for (int r = 1; r < rows-1; r++) {
        if (words_per_row == 1) {
            mask[r] = (~0ULL << leftpad) & (~0ULL >> rightpad);
        } else {
            mask[r * words_per_row] = (~0ULL << leftpad);
            mask[r * words_per_row + words_per_row - 1] = (~0ULL >> rightpad);
            for (int w = 1; w < words_per_row-1; w++) {
                mask[r * words_per_row + w] = ~0ULL;
            }
        }
    }
}

void Grid::initCheckerGrid() {
    uint64_t word = 0x5555555555555555;

    for (int r = 0; r < rows; ++r) {
        for (int w = 0; w < words_per_row; ++w){
            int idx = r * words_per_row + w;
            if ((r % 2) == 0) {
                current[idx] = word & mask[idx];
            } else {
                current[idx] = ~word & mask[idx];
            }
        }
    }
}

void Grid::initRandomGrid() {
    std::vector<uint64_t> random_buffer(current.size());
    for (auto &val : random_buffer)
        val = dist(rng);

    for (size_t i = 0; i < current.size(); ++i) {
        current[i] = random_buffer[i] & mask[i];
    }
}

void Grid::step() {
    for (int rb = 1; rb < rows - 1; rb += blocksize) {
        int rend = std::min(rows - 1, rb + blocksize);
        for (int r = rb; r < rend; ++r) {
            const uint64_t* top = &current[(r-1)*words_per_row];
            const uint64_t* mid = &current[r*words_per_row];
            const uint64_t* bot = &current[(r+1)*words_per_row];
            const uint64_t* row_mask = &mask[r*words_per_row];
            uint64_t* out = &next[r*words_per_row];

            for (int w = 0; w < words_per_row; ++w) {
                uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                uint64_t c1 = 0, c2 = 0;
                uint64_t is2 = 0;
                uint64_t is3 = 0;

                uint64_t top_left  = (top[w] << 1) | (w>0 ? top[w-1] >> 63 : 0);
                uint64_t top_mid   = top[w];
                uint64_t top_right = (top[w] >> 1) | (w<words_per_row-1 ? top[w+1] << 63 : 0);

                uint64_t mid_left  = (mid[w] << 1) | (w>0 ? mid[w-1] >> 63 : 0);
                uint64_t mid_right = (mid[w] >> 1) | (w<words_per_row-1 ? mid[w+1] << 63 : 0);

                uint64_t bot_left  = (bot[w] << 1) | (w>0 ? bot[w-1] >> 63 : 0);
                uint64_t bot_mid   = bot[w];
                uint64_t bot_right = (bot[w] >> 1) | (w<words_per_row-1 ? bot[w+1] << 63 : 0);

                // bitwise addition

                c1 = s0 & top_left; s0 ^= top_left; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & top_mid;  s0 ^= top_mid;  c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & top_right; s0 ^= top_right; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & mid_left; s0 ^= mid_left; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & mid_right; s0 ^= mid_right; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & bot_left; s0 ^= bot_left; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & bot_mid; s0 ^= bot_mid; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & bot_right; s0 ^= bot_right; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;

                is2 = (~s3) & (~s2) & s1 & ~s0;
                is3 = (~s3) & (~s2) & s1 &  s0;

                out[w] = (is3 | (mid[w] & is2)) & row_mask[w];
            }
        }
    }
    std::swap(current, next);
    //unpackGrid();
}

std::vector<uint64_t> Grid::getGrid() {
    return current;
}

std::vector<uint64_t> Grid::getMask() {
    return mask;
}

const uint32_t* Grid::getGrid32Ptr() const {
    return reinterpret_cast<const uint32_t*>(current.data());
}

void Grid::printMask() {
    for (int r = 0; r < rows; ++r) {
        for (int w = 0; w < words_per_row; ++w) {
            for (int b = 0; b < 64; ++b) {
                std::cout << (mask[r * words_per_row + w] & (1ULL << b) ? '#' : '.');
            }
        }
        std::cout << "\n";
    }
}

void Grid::printCurrent() {
    for (int r = 0; r < rows; ++r) {
        for (int w = 0; w < words_per_row; ++w) {
            for (int b = 0; b < 64; ++b) {
                std::cout << (current[r * words_per_row + w] & (1ULL << b) ? '#' : '.');
            }
        }
        std::cout << "\n";
    }
}