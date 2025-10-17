#include "controller.hpp"

Controller::Controller(Config* cfg, Window* win, Grid* grid, Console* console, Renderer* renderer)
    : cfg(cfg), win(win), grid(grid), console(console), renderer(renderer) 
{

}

void Controller::setWindowSize(int w, int h) {
    cfg->width = w;
    cfg->height = h;
    if (cfg->window) glfwSetWindowSize(cfg->window, w, h);
}

void Controller::setGridSize(int x, int y) {
    cfg->gridx = x;
    cfg->gridy = y;
    grid->initSize();
    grid->initMask();
    grid->initRandomGrid();
    renderer->reset();
    renderer->render();
}

void Controller::regen() {
    grid->initRandomGrid();
}

void Controller::start() {
    grid->pause = false;
    if (cfg->vsync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }
}

void Controller::stop() {
    grid->pause = true;
    glfwSwapInterval(1);
}

void Controller::step(int n_step, float delay) {
    for (int i = 0; i < n_step; ++i) {
        grid->step();
        renderer->render();
        console->draw(win->get());
        glfwSwapBuffers(win->get());
        
    }
}