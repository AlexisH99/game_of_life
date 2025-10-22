#include "renderer.hpp"
#include "shaders_sources.hpp"
#include <iostream>
#include <algorithm>



Renderer::Renderer(const Grid* grid, const Config* cfg) {
    this->cfg = cfg;
    this->grid = grid;

    vertices.resize(24);
    vao = std::make_unique<GLVertexBuffer>();
    vbo = std::make_unique<GLBuffer>(GL_ARRAY_BUFFER);
    texture = std::make_unique<GLTextureBuffer>();
    shaders = std::make_unique<GLProgram>(mainVert, mainFrag);
}

void Renderer::initRender() {
    float gridAspect   = (float)cfg->gridx / (float)cfg->gridy;
    float windowAspect = (float)cfg->width / (float)cfg->height;

    if (windowAspect > gridAspect) {
        // Fenêtre plus large : bandes sur les côtés
        vertHeight = 1.0f;
        vertWidth  = gridAspect / windowAspect;
    } else {
        // Fenêtre plus haute : bandes en haut/bas
        vertWidth  = 1.0f;
        vertHeight = windowAspect / gridAspect;
    }

    vertices = {
        // pos       // texcoords
        -vertWidth,  vertHeight,  0.0f, 1.0f, // haut gauche
        -vertWidth, -vertHeight,  0.0f, 0.0f, // bas gauche
        vertWidth, -vertHeight,  1.0f, 0.0f, // bas droit

        -vertWidth,  vertHeight,  0.0f, 1.0f, // haut gauche
        vertWidth, -vertHeight,  1.0f, 0.0f, // bas droit
        vertWidth,  vertHeight,  1.0f, 1.0f  // haut droit
    };

    vao->bind();
    vbo->bind();


    vbo->set_data(vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    texture->bind(0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    texture->allocate(GL_RG32UI, grid->rows * grid->words_per_row * sizeof(uint32_t) * 2, grid->getGrid32Ptr());
}

void Renderer::render() {
    texture->update(grid->rows * grid->words_per_row * sizeof(uint32_t) * 2, grid->getGrid32Ptr());

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    shaders->use();
    texture->bind(0);
    glUniform1i(glGetUniformLocation(shaders->get(), "packedGrid"), 0);
    glUniform1i(glGetUniformLocation(shaders->get(), "leftpad"), grid->leftpad);
    glUniform2f(glGetUniformLocation(shaders->get(), "windowSize"), cfg->width, cfg->height);
    glUniform2f(glGetUniformLocation(shaders->get(), "gridSize"), cfg->gridx, cfg->gridy);
    glUniform1i(glGetUniformLocation(shaders->get(), "words_per_row"), grid->words_per_row);
    glUniform1f(glGetUniformLocation(shaders->get(), "zoom"), zoom);
    glUniform2f(glGetUniformLocation(shaders->get(), "camera"), camX, camY);
    vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}