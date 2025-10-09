#include "grid.hpp"

Grid::Grid() : rng(std::random_device{}()), dist(0, 1) {

}

Grid::~Grid() {

}

void Grid::initSize(int gridx, int gridy) {
    gridX = gridx; gridY = gridy;
    current.resize(gridX * gridY);
    next.resize(gridX * gridY);
}

void Grid::checkerInit() {
    for (int y = 0; y < gridY; ++y) {
        for (int x = 0; x < gridX; ++x) {
            int idx = y * gridX + x;
            // Case blanche si (x+y) pair, noire sinon
            current[idx] = ((x + y) % 2 == 0) ? 255 : 0;
        }
    }
}

void Grid::randomInit() {
    for (int i = 0; i < gridX * gridY; i++) {
        current[i] = dist(rng) ? 255 : 0;
    }
}

void Grid::step() {
    auto getCell = [&](int x, int y) -> int {
        if (x < 0 || y < 0 || x >= gridX || y >= gridY) return 0; // bords = morts
        return current[y * gridX + x] > 0; // 0 ou 1
    };

    for (int y = 0; y < gridY; ++y) {
        for (int x = 0; x < gridX; ++x) {
            int alive = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    alive += getCell(x + dx, y + dy);
                }
            }
            int idx = y * gridX + x;
            if (current[idx]) {
                next[idx] = (alive == 2 || alive == 3) ? 255 : 0;
            } else {
                next[idx] = (alive == 3) ? 255 : 0;
            }
        }
    }

    current.swap(next);
}

std::vector<uint8_t> Grid::get() {
    return current;
}