#pragma once

#include "gl_wrappers.hpp"
#include "config.hpp"
#include "grid.hpp"

#include <memory>

class Renderer {
    public:
        Renderer(const Grid* grid, const Config* cfg);

        void initRender();
        void render();

        float zoom = 1.0f;
        float camX = 0.0f;   // coin haut-gauche visible (en cellules)
        float camY = 0.0f;
        float zoomSpeed = 1.05f;  // sensibilité de la molette
        float minZoom = 1.0f;
        float maxZoom = 100.0f;

    private:
        const uint32_t* grid_data;
        std::vector<float> vertices;

        float vertWidth = 0.0f;
        float vertHeight = 0.0f;

        float widthRatio = 0.0f;
        float heightRatio = 0.0f;

        const Grid* grid = nullptr;
        const Config* cfg = nullptr;

        std::unique_ptr<GLVertexBuffer> vao;
        std::unique_ptr<GLBuffer> vbo;
        std::unique_ptr<GLTextureBuffer> texture;
        std::unique_ptr<GLProgram> shaders;
};