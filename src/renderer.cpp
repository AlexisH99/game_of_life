#include "renderer.hpp"
#include "shaders_sources.hpp"


Renderer::Renderer(const Grid* grid, const Config* cfg) {
    this->cfg = cfg;
    this->grid = grid;

    vao = std::make_unique<GLVertexBuffer>();
    vbo = std::make_unique<GLBuffer>(GL_ARRAY_BUFFER);
    texture = std::make_unique<GLTexture>(GL_TEXTURE_2D);
    shaders = std::make_unique<GLProgram>(mainVert, mainFrag);

    float vertices[] = {
        // pos       // texcoords
        -1.0f,  1.0f,  0.0f, 1.0f, // haut gauche
        -1.0f, -1.0f,  0.0f, 0.0f, // bas gauche
        1.0f, -1.0f,  1.0f, 0.0f, // bas droit

        -1.0f,  1.0f,  0.0f, 1.0f, // haut gauche
        1.0f, -1.0f,  1.0f, 0.0f, // bas droit
        1.0f,  1.0f,  1.0f, 1.0f  // haut droit
    };

    vao->bind();
    vbo->bind();

    vbo->set_data(sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    texture->bind(0);
    texture->set_parameters();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    texture->allocate(GL_RG32UI, grid->words_per_row, grid->rows, GL_RG_INTEGER, GL_UNSIGNED_INT, grid->getGrid32Ptr());
}

void Renderer::reset() {
    texture->bind(0);
    texture->set_parameters();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    texture->allocate(GL_RG32UI, grid->words_per_row, grid->rows, GL_RG_INTEGER, GL_UNSIGNED_INT, grid->getGrid32Ptr());
}

void Renderer::render() {
    texture->set_data(grid->words_per_row, grid->rows, GL_RG_INTEGER, GL_UNSIGNED_INT, grid->getGrid32Ptr());

    glClearColor(1.0f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    shaders->use();
    glUniform1i(glGetUniformLocation(shaders->get(), "packedGrid"), 0);
    glUniform1i(glGetUniformLocation(shaders->get(), "leftpad"), grid->leftpad);
    glUniform2f(glGetUniformLocation(shaders->get(), "windowSize"), cfg->width, cfg->height);
    glUniform2f(glGetUniformLocation(shaders->get(), "gridSize"), cfg->gridx, cfg->gridy);
    vao->bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}