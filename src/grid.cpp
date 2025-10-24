#include "grid.hpp"

#include <iostream>
#include <cstdlib>
#include <omp.h>
#include <immintrin.h>

Grid::Grid() : rng(std::random_device{}()), uniform_dist(0, ~0ULL), bernoulli_dist(0.5) {

}

Grid::~Grid() {

}

// Rng init with random or given seed
void Grid::initSeed() {
    if (cfg->randomSeed) {
        gridSeed = std::random_device{}();
    } else {
        gridSeed = cfg->seed;
    }
    rng.seed(gridSeed);
}

// Init size of every buffer related to grid
void Grid::initSize() {
    omp_set_num_threads(4);

    rows = cfg->gridy + 2;
    words_per_row = w_for_w(cfg->gridx);
    blocksize = 64;
    
    mask.resize(rows * words_per_row);
    current.resize(rows * words_per_row);
    next.resize(rows * words_per_row);
    
    mask.assign(rows * words_per_row, 0ULL);
    current.assign(rows * words_per_row, 0ULL);
    next.assign(rows * words_per_row, 0ULL);

    count.resize(9);
}

// Init born and survive masks
void Grid::initRuleset() {
    born_rule = cfg->born_rule;
    survive_rule = cfg->survive_rule;
}

// Init grid mask
void Grid::initMask() {
    int pad = ((words_per_row * 64) - cfg->gridx);
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

// Init the grid as a checkerboard, for debug purposes
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

// Init the grid as random
void Grid::initRandomGrid() {
    if (cfg->distType == "uniform") {
        for (size_t i = 0; i < current.size(); ++i) {
            current[i] = uniform_dist(rng) & mask[i];
        }
    } else if (cfg->distType == "bernoulli") {
        bernoulli_dist = std::bernoulli_distribution(cfg->density);
        uint64_t word = 0ULL;
        for (size_t i = 0; i < current.size(); ++i) {
            word = 0ULL;
            for (int bit = 0; bit < 64; ++bit) {
                if (bernoulli_dist(rng)) word |= (1ULL << bit);
            }
            current[i] = word & mask[i];
        }
    } else {
        throw std::runtime_error("[Fatal] Bad type error: " + cfg->distType);
    }
}

// Step function
/* void Grid::step() {
    // Loop by row blocks for future multithread, currently the blocksize is locked to one, so this loop is neutral
    #pragma omp parallel
    {
    #pragma omp for schedule(static,8)
    for (int rb = 1; rb < rows - 1; rb += blocksize) {
        int rend = std::min(rows - 1, rb + blocksize);
        for (int r = rb; r < rend; ++r) {
            // Load top, mid (current) and bottom rows, mask row and out buffer pointers
            const uint64_t* top = &current[(r-1)*words_per_row];
            const uint64_t* mid = &current[r*words_per_row];
            const uint64_t* bot = &current[(r+1)*words_per_row];
            const uint64_t* row_mask = &mask[r*words_per_row];
            uint64_t* out = &next[r*words_per_row];

            // Loop through each word of the rows, and shifting in each direction to get Moore's neighborhood
            for (int w = 0; w < words_per_row; ++w) {
                uint64_t top_left  = (top[w] << 1) | (w>0 ? top[w-1] >> 63 : 0);
                uint64_t top_mid   = top[w];
                uint64_t top_right = (top[w] >> 1) | (w<words_per_row-1 ? top[w+1] << 63 : 0);

                uint64_t mid_left  = (mid[w] << 1) | (w>0 ? mid[w-1] >> 63 : 0);
                uint64_t mid_right = (mid[w] >> 1) | (w<words_per_row-1 ? mid[w+1] << 63 : 0);

                uint64_t bot_left  = (bot[w] << 1) | (w>0 ? bot[w-1] >> 63 : 0);
                uint64_t bot_mid   = bot[w];
                uint64_t bot_right = (bot[w] >> 1) | (w<words_per_row-1 ? bot[w+1] << 63 : 0);

                // Bitwise adder
                uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                uint64_t c1 = 0, c2 = 0;

                c1 = s0 & top_left; s0 ^= top_left; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & top_mid;  s0 ^= top_mid;  c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & top_right; s0 ^= top_right; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & mid_left; s0 ^= mid_left; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & mid_right; s0 ^= mid_right; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & bot_left; s0 ^= bot_left; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & bot_mid; s0 ^= bot_mid; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & bot_right; s0 ^= bot_right; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;

                // Count buffer: 0 to 8 neighboors for each cell of the current word
                count[0] = ~s3 & ~s2 & ~s1 & ~s0;
                count[1] = ~s3 & ~s2 & ~s1 & s0;
                count[2] = ~s3 & ~s2 & s1 & ~s0;
                count[3] = ~s3 & ~s2 & s1 & s0;
                count[4] = ~s3 & s2 & ~s1 & ~s0;
                count[5] = ~s3 & s2 & ~s1 & s0;
                count[6] = ~s3 & s2 & s1 & ~s0;
                count[7] = ~s3 & s2 & s1 & s0;
                count[8] = s3 & ~s2 & ~s1 & ~s0;

                // Set born and survive masks for the word
                uint64_t born = 0ULL, survive = 0ULL;

                // Compute the born and survive condition given the count buffer and the born and survive rules
                for (int i = 0; i < 9; ++i) {
                    if (born_rule & (1U << i)) born |= count[i];
                    if (survive_rule & (1U << i)) survive |= count[i];
                }

                // Final output with born and survive conditions, given the current state of the cells and the mask
                out[w] = (born | (mid[w] & survive)) & row_mask[w];
            }
        }
    }
    }// Swap current and next buffers
    std::swap(current, next);
} */

void Grid::step() {
    const int VEC_WIDTH = 4; // 4x uint64_t = 256 bits

    #pragma omp parallel
    {
        #pragma omp for schedule(static)
        for (int r = 1; r < rows - 1; ++r) {
            const uint64_t* top = &current[(r - 1) * words_per_row];
            const uint64_t* mid = &current[r * words_per_row];
            const uint64_t* bot = &current[(r + 1) * words_per_row];
            const uint64_t* row_mask = &mask[r * words_per_row];
            uint64_t* out = &next[r * words_per_row];

            int w = 0;

            // === 1. Boucle vectorisée AVX2 ===
            for (; w <= words_per_row - VEC_WIDTH; w += VEC_WIDTH) {
                // Charger 4 mots de chaque ligne
                __m256i top_w  = _mm256_loadu_si256((__m256i*)&top[w]);
                __m256i mid_w  = _mm256_loadu_si256((__m256i*)&mid[w]);
                __m256i bot_w  = _mm256_loadu_si256((__m256i*)&bot[w]);
                __m256i mask_w = _mm256_loadu_si256((__m256i*)&row_mask[w]);

                // Décalages gauche et droite par bit
                __m256i top_left  = _mm256_or_si256(_mm256_slli_epi64(top_w, 1),
                                   _mm256_srli_epi64(_mm256_alignr_epi8(top_w, _mm256_permute4x64_epi64(top_w, _MM_SHUFFLE(2,1,0,3)), 8), 63));
                __m256i top_right = _mm256_or_si256(_mm256_srli_epi64(top_w, 1),
                                   _mm256_slli_epi64(_mm256_alignr_epi8(_mm256_permute4x64_epi64(top_w, _MM_SHUFFLE(0,3,2,1)), top_w, 24), 63));

                __m256i mid_left  = _mm256_or_si256(_mm256_slli_epi64(mid_w, 1),
                                   _mm256_srli_epi64(_mm256_alignr_epi8(mid_w, _mm256_permute4x64_epi64(mid_w, _MM_SHUFFLE(2,1,0,3)), 8), 63));
                __m256i mid_right = _mm256_or_si256(_mm256_srli_epi64(mid_w, 1),
                                   _mm256_slli_epi64(_mm256_alignr_epi8(_mm256_permute4x64_epi64(mid_w, _MM_SHUFFLE(0,3,2,1)), mid_w, 24), 63));

                __m256i bot_left  = _mm256_or_si256(_mm256_slli_epi64(bot_w, 1),
                                   _mm256_srli_epi64(_mm256_alignr_epi8(bot_w, _mm256_permute4x64_epi64(bot_w, _MM_SHUFFLE(2,1,0,3)), 8), 63));
                __m256i bot_right = _mm256_or_si256(_mm256_srli_epi64(bot_w, 1),
                                   _mm256_slli_epi64(_mm256_alignr_epi8(_mm256_permute4x64_epi64(bot_w, _MM_SHUFFLE(0,3,2,1)), bot_w, 24), 63));

                // === 2. Addition binaire (compteur de voisins) ===
                __m256i s0 = _mm256_setzero_si256();
                __m256i s1 = _mm256_setzero_si256();
                __m256i s2 = _mm256_setzero_si256();
                __m256i s3 = _mm256_setzero_si256();
                __m256i c1, c2;

                auto add_bit = [&](const __m256i& b) {
                    c1 = _mm256_and_si256(s0, b);  s0 = _mm256_xor_si256(s0, b);
                    c2 = _mm256_and_si256(s1, c1); s1 = _mm256_xor_si256(s1, c1);
                    s3 = _mm256_xor_si256(s3, _mm256_and_si256(s2, c2)); s2 = _mm256_xor_si256(s2, c2);
                };

                add_bit(top_left);  add_bit(top_w);   add_bit(top_right);
                add_bit(mid_left);  add_bit(mid_right);
                add_bit(bot_left);  add_bit(bot_w);   add_bit(bot_right);

                // === 3. Conversion en masques count[0..8] ===
                __m256i not_s0 = _mm256_xor_si256(s0, _mm256_set1_epi64x(-1));
                __m256i not_s1 = _mm256_xor_si256(s1, _mm256_set1_epi64x(-1));
                __m256i not_s2 = _mm256_xor_si256(s2, _mm256_set1_epi64x(-1));
                __m256i not_s3 = _mm256_xor_si256(s3, _mm256_set1_epi64x(-1));

                __m256i count[9];
                count[0] = _mm256_and_si256(_mm256_and_si256(_mm256_and_si256(not_s3, not_s2), not_s1), not_s0);
                count[1] = _mm256_and_si256(_mm256_and_si256(_mm256_and_si256(not_s3, not_s2), not_s1), s0);
                count[2] = _mm256_and_si256(_mm256_and_si256(_mm256_and_si256(not_s3, not_s2), s1), not_s0);
                count[3] = _mm256_and_si256(_mm256_and_si256(_mm256_and_si256(not_s3, not_s2), s1), s0);
                count[4] = _mm256_and_si256(_mm256_and_si256(_mm256_and_si256(not_s3, s2), not_s1), not_s0);
                count[5] = _mm256_and_si256(_mm256_and_si256(_mm256_and_si256(not_s3, s2), not_s1), s0);
                count[6] = _mm256_and_si256(_mm256_and_si256(_mm256_and_si256(not_s3, s2), s1), not_s0);
                count[7] = _mm256_and_si256(_mm256_and_si256(_mm256_and_si256(not_s3, s2), s1), s0);
                count[8] = _mm256_and_si256(_mm256_and_si256(_mm256_and_si256(s3, not_s2), not_s1), not_s0);

                // === 4. Règle Born/Survive ===
                __m256i born = _mm256_setzero_si256();
                __m256i survive = _mm256_setzero_si256();
                for (int i = 0; i < 9; ++i) {
                    if (born_rule & (1U << i))    born    = _mm256_or_si256(born, count[i]);
                    if (survive_rule & (1U << i)) survive = _mm256_or_si256(survive, count[i]);
                }

                // === 5. Résultat final ===
                __m256i out_w = _mm256_and_si256(
                    _mm256_or_si256(born, _mm256_and_si256(mid_w, survive)),
                    mask_w
                );

                // Écriture
                _mm256_storeu_si256((__m256i*)&out[w], out_w);
            }

            // === 6. Reste scalaire (si non multiple de 4) ===
            for (; w < words_per_row; ++w) {
                uint64_t top_left  = (top[w] << 1) | (w>0 ? top[w-1] >> 63 : 0);
                uint64_t top_mid   = top[w];
                uint64_t top_right = (top[w] >> 1) | (w<words_per_row-1 ? top[w+1] << 63 : 0);

                uint64_t mid_left  = (mid[w] << 1) | (w>0 ? mid[w-1] >> 63 : 0);
                uint64_t mid_right = (mid[w] >> 1) | (w<words_per_row-1 ? mid[w+1] << 63 : 0);

                uint64_t bot_left  = (bot[w] << 1) | (w>0 ? bot[w-1] >> 63 : 0);
                uint64_t bot_mid   = bot[w];
                uint64_t bot_right = (bot[w] >> 1) | (w<words_per_row-1 ? bot[w+1] << 63 : 0);

                uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
                uint64_t c1 = 0, c2 = 0;

                c1 = s0 & top_left; s0 ^= top_left; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & top_mid;  s0 ^= top_mid;  c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & top_right; s0 ^= top_right; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & mid_left; s0 ^= mid_left; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & mid_right; s0 ^= mid_right; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & bot_left; s0 ^= bot_left; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & bot_mid; s0 ^= bot_mid; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;
                c1 = s0 & bot_right; s0 ^= bot_right; c2 = s1 & c1; s1 ^= c1; s3 ^= s2 & c2; s2 ^= c2;

                uint64_t count[9];
                count[0] = ~s3 & ~s2 & ~s1 & ~s0;
                count[1] = ~s3 & ~s2 & ~s1 & s0;
                count[2] = ~s3 & ~s2 & s1 & ~s0;
                count[3] = ~s3 & ~s2 & s1 & s0;
                count[4] = ~s3 & s2 & ~s1 & ~s0;
                count[5] = ~s3 & s2 & ~s1 & s0;
                count[6] = ~s3 & s2 & s1 & ~s0;
                count[7] = ~s3 & s2 & s1 & s0;
                count[8] = s3 & ~s2 & ~s1 & ~s0;

                uint64_t born = 0ULL, survive = 0ULL;
                for (int i = 0; i < 9; ++i) {
                    if (born_rule & (1U << i)) born |= count[i];
                    if (survive_rule & (1U << i)) survive |= count[i];
                }

                out[w] = (born | (mid[w] & survive)) & row_mask[w];
            }
        }
    }

    std::swap(current, next);
}

// Get raw grid content
std::vector<uint64_t> Grid::getGrid() {
    return current;
}

// Get the mask
std::vector<uint64_t> Grid::getMask() {
    return mask;
}

// Get the grid as a uint32_t recast for texture rendering
const uint32_t* Grid::getGrid32Ptr() const {
    return reinterpret_cast<const uint32_t*>(current.data());
}

// Function to print the mask, for debug purposes
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

// Function to print the grid, for debug purposes
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