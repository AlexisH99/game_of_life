#pragma once
#include "config.hpp"
#include "window.hpp"
#include "grid.hpp"
#include "console.hpp"
#include "renderer.hpp"

class Controller {
    public:
        Controller(Config* cfg, Window* win, Grid* grid, Console* console, Renderer* renderer);

        int getWindowWidth() { return cfg->width; }
        int getWindowHeight() { return cfg->height; }
        void setWindowSize(int w, int h);
        void setGridSize(int x, int y);
        
        void regen();
        void start();
        void stop();
        void step(int n_step = 1, float delay = 0.0);

    private:
        Config* cfg;
        Window* win;
        Grid* grid;
        Console* console;
        Renderer* renderer;
};