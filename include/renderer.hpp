#pragma once

#include "gl_wrappers.hpp"
#include "config.hpp"
#include "grid.hpp"
#include <memory>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <format>

class Renderer {
    public:
        Renderer(const Grid* grid, const Config* cfg);

        void render();

    private:
        int words_per_row, rows;
        const uint32_t* grid_data;

        const Grid* grid = nullptr;
        const Config* cfg = nullptr;

        std::unique_ptr<GLVertexBuffer> vao;
        std::unique_ptr<GLBuffer> vbo;
        std::unique_ptr<GLTexture> texture;
        std::unique_ptr<GLProgram> shaders;
};