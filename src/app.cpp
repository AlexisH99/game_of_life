#include "app.hpp"
#include "GLFW/glfw3.h"

#include <algorithm>
#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <format>

Application::Application() {

}

Application::~Application() {
    
}

void Application::run() {
    loadConfig();
    initWindow();
    initGlad();
    initGrid();
    initRender();
    initConsole();
    mainLoop();
}

// When window change size, get width and height of the viewport, update the value in Config class and reset the Rendering class.
void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        glViewport(0, 0, width, height);
        app->cfg->width = width;
        app->cfg->height = height;
        app->renderer->initRender();
    }
}

// Function to load application icon from inside the executable
void Application::set_window_icon_from_resource(GLFWwindow* window) {
    HICON hIcon = (HICON)LoadImage(
        GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDI_APP_ICON),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTSIZE
    );

    if (!hIcon) {
        std::cerr << "[Resource Error] cannot load resource from executable" << std::endl;
        return;
    }

    ICONINFO iconInfo = {};
    GetIconInfo(hIcon, &iconInfo);

    BITMAP bmp = {};
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);

    int width = bmp.bmWidth;
    int height = bmp.bmHeight;

    std::vector<unsigned char> pixels(width * height * 4);
    GetBitmapBits(iconInfo.hbmColor, width * height * 4, pixels.data());

    for (int y = 0; y < height / 2; ++y) {
        for (int x = 0; x < width * 4; ++x) {
            std::swap(pixels[y * width * 4 + x],
                      pixels[(height - 1 - y) * width * 4 + x]);
        }
    }
    for (size_t i = 0; i < pixels.size(); i += 4) {
        std::swap(pixels[i], pixels[i + 2]);  // B <-> R
    }

    GLFWimage img;
    img.width = width;
    img.height = height;
    img.pixels = pixels.data();

    glfwMakeContextCurrent(window);
    glfwSetWindowIcon(window, 1, &img);

    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    DestroyIcon(hIcon);
}

// Key callback function, with space to pause/resume simulation, right arrow key to do one step, F1 to open/close the console, escapte to close the console.
// If console is visible, all the keys are redirected to the handleInput function of the Console class
void Application::key_callback(GLFWwindow* window, int key, [[maybe_unused]]int scancode, [[maybe_unused]]int action, [[maybe_unused]]int mods)
{   
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    glfwMakeContextCurrent(window);
    if (app) {
        // Pause/resume condition. vsync is activated when paused to limit useless framerate
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && !app->console->visible) {
            app->grid->pause = !app->grid->pause;
            if (app->grid->pause) {
                glfwSwapInterval(1);
            } else {
                if (!(app->cfg->vsync)) {
                    glfwSwapInterval(0);
                }
            }
        }

        // If the simulation is paused, right arrow key press do one simulation step, like a trigger.
        if (app->grid->pause && key == GLFW_KEY_RIGHT && action == GLFW_PRESS && !app->console->visible) {
            app->grid->step();
        }

        // Show/hide console condition : it juste changes the visible bool in Console class to not itself
        if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
            app->console->visible = !app->console->visible;
        }

        // Force console hiding with Escape key because I was always trying to close it with escape
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            app->console->visible = false;
        }

        // If the console is visible, redirect all the entries in the handleInput function of Console class
        if (app->console->visible) {
            app->console->handleInput(key, action);
        }
    }
}

// callback to handle characters by codepoints, to get it in the font file
void Application::char_callback(GLFWwindow* window, unsigned int codepoint){
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->console->handleChar(codepoint);
}

// Mouse scroll callback function to control zoom when the mouse is on the grid, or scroll console when mouse is on the console
void Application::scroll_callback(GLFWwindow* window, [[maybe_unused]]double xoffset, double yoffset) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    if (!app || !app->renderer) return;

    // Locally get pointers to Renderer, Config and Console classes
    Renderer* r = app->renderer.get();
    Config* c = app->cfg.get();
    Console* cons = app->console.get();
    
    // mouseX and mouseY are the current cursor positions
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // When console not visible or mouse under the console
    if (!cons->visible || mouseY > cons->cHeight) {
        // Vertical flip of the coordinate in Y axis because Y coordinate of pixels are flipped in fragment shaders
        mouseY = c->height - mouseY;

        // Get mouse coordinates in the grid coordinate system before zoom
        float gridX_before = r->camX + (mouseX / c->width)  * (c->gridx / r->zoom);
        float gridY_before = r->camY + (mouseY / c->height) * (c->gridy / r->zoom);

        // Update zoom value with yoffset (mouse scroll or vertical pad scroll)
        r->zoom *= pow(r->zoomSpeed, yoffset);

        // Compute max zoom according to horizontal and vertical cells per pixel ratio
        float maxZoom = 100.0f * std::max(
            (float)c->gridx / (float)c->width,
            (float)c->gridy / (float)c->height
        );

        // Clamp zoom value from 1 (no zoom, full grid view) to max zoom value
        r->zoom = std::clamp(r->zoom, r->minZoom, maxZoom);

        // Get mouse coordinate in the grid coordinates system with the new zoom
        float gridX_after = r->camX + (mouseX / c->width)  * (c->gridx / r->zoom);
        float gridY_after = r->camY + (mouseY / c->height) * (c->gridy / r->zoom);

        // Translate camera with the difference to cancel the displacement due to the zoom
        // If you zoom on any cell of the grid, it will stay under the mouse
        r->camX += (gridX_before - gridX_after);
        r->camY += (gridY_before - gridY_after);

        // Compute the visible width and height with new zoom value
        float visibleWidth  = c->gridx / r->zoom;
        float visibleHeight = c->gridy / r->zoom;

        // Clamp camera position in the two axes between 0 and the maximum (grid size minus visible range)
        r->camX = std::clamp(r->camX, 0.0f, c->gridx - visibleWidth);
        r->camY = std::clamp(r->camY, 0.0f, c->gridy - visibleHeight);

    } else { // When console is visible and the mouse is on the console
        // Set scrollAccumulator of Console class by adding the yoffset to it
        cons->scrollAccumulator += yoffset;
        const double threshold = 1.0f;

        // When scroll accumulator is over the threshold, reduce it 1 by 1 and remove 1 to line offset each time
        // This scrolls up
        while (cons->scrollAccumulator >= threshold) {
            cons->lineOffset -= 1;
            cons->scrollAccumulator -= threshold;
        }

        // The same as previous block but the opposite so this scrolls down
        while (cons->scrollAccumulator <= threshold) {
            cons->lineOffset += 1;
            cons->scrollAccumulator += threshold;
        }

        // Compute the maxScroll value and clamp line offset between 0 to maxScroll
        int maxScroll = std::max(0, (int)cons->lines.size() - cons->maxVisibleLines);
        cons->lineOffset = std::clamp(cons->lineOffset, 0, maxScroll);
    }
}

// Cursor position callback function used to control the padding of the grid in x and y
void Application::cursor_position_callback(GLFWwindow* window, double posX, double posY) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app || !app->renderer) return;

    // Get pointers to Renderer and Config classes
    Renderer* r = app->renderer.get();
    Config* c = app->cfg.get();

    // Set current posX and poY as last ones
    static double last_posX = posX, last_posY = posY;

    // On left click do:
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        // Compute the cursor displacement
        double dx = posX - last_posX;
        double dy = posY - last_posY;

        // Get cells per pixel in both dimensions
        float cellPerPixelX = (c->gridx / r->zoom) / c->width;
        float cellPerPixelY = (c->gridy / r->zoom) / c->height;

        // Update cam position in both dimensions (Y is flipped)
        r->camX -= dx * cellPerPixelX;
        r->camY += dy * cellPerPixelY;

        // Get visible width and height
        float visibleWidth  = c->gridx / r->zoom;
        float visibleHeight = c->gridy / r->zoom;

        // Clamp cam positions between 0 and grid size minus visible range
        r->camX = std::clamp(r->camX, 0.0f, c->gridx - visibleWidth);
        r->camY = std::clamp(r->camY, 0.0f, c->gridy - visibleHeight);
    }

    last_posX = posX;
    last_posY = posY;
}

// Initial config loader : Loads the Config class, read the config.jsonc file or create it if not present and print param in console
// If width and height are below 800 and 600 respectively, send a warning message and move back those values to minimum required
void Application::loadConfig() {
    cfg = std::make_unique<Config>();
    cfg->initConfig("config.jsonc");
    cfg->printAllParams();
    if (cfg->width < 800) {
        std::cout << "[Config Warning] minimum allowed width is 800. Moved back to this value.\n";
        cfg->width = 800;
    }
    if (cfg->height < 600) {
        std::cout << "[Config Warning] minimum allowed height is 600. Moved back to this value.\n";
        cfg->height = 600;
    }
}

// Window class loader with some glfw setup (vsync, callbacks, ...)
void Application::initWindow() {
    window = std::make_unique<Window>(cfg->width, cfg->height, title);
    if (!window) throw std::runtime_error("[Runtime Error] Cannot initialize window context");
    cfg->window = window->get();
    window->makeContextCurrent();
    window->setUserPointer(this);
    set_window_icon_from_resource(window->get());
    
    glfwSetKeyCallback(window->get(), key_callback);
    glfwSetCharCallback(window->get(), char_callback);
    glfwSetScrollCallback(window->get(), scroll_callback);
    glfwSetCursorPosCallback(window->get(), cursor_position_callback);
    
    if (cfg->vsync || (cfg->freeze_at_start && !cfg->vsync)) {
        glfwSwapInterval(1); // Vsync ON
    } else { 
        glfwSwapInterval(0); // Vsync OFF
    }
}

// GLAD loader with some hardware compatibility checks and framebuffer size setup
void Application::initGlad() {
    if (!gladLoadGL(glfwGetProcAddress)) throw std::runtime_error("[Runtime Error] Cannot initialize GLAD");

    // Check OpenGL 3.3 support
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "[Info] OpenGL version: " << version << std::endl;
    if (!GLAD_GL_VERSION_3_3) throw std::runtime_error("[Runtime Error] OpenGL 3.3 or higher required.");

    // Check GL_RG32UI texture integer support
    GLint supported = 0;
    glGetInternalformativ(GL_TEXTURE_2D, GL_RG32UI, GL_INTERNALFORMAT_SUPPORTED, 1, &supported);
    if (!supported) throw std::runtime_error("[Runtime Error] GL_RG32UI not supported on this GPU.");

    glfwGetFramebufferSize(window->get(), &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glfwSetFramebufferSizeCallback(window->get(), framebuffer_size_callback);
}

// Grid loader
void Application::initGrid() {
    grid = std::make_unique<Grid>();
    if (!grid) throw std::runtime_error("[Runtime Error] Cannot initialize grid");
    grid->cfg = cfg.get();
    grid->pause = cfg->freeze_at_start;
    grid->initSeed();
    grid->initRuleset();
    grid->initSize();
    grid->initMask();
    if (cfg->checker == true) {
        grid->initCheckerGrid();
    } else {
        grid->initRandomGrid();
    }
}

// Renderer loader
void Application::initRender() {
    renderer = std::make_unique<Renderer>(grid.get(), cfg.get());
    if (!renderer) throw std::runtime_error("[Runtime Error] Cannot initialize renderer");
    renderer->initRender();
}

// Console loader
void Application::initConsole() {
    console = std::make_unique<Console>(cfg.get(), window.get(), grid.get(), renderer.get());
    if (!console) throw std::runtime_error("[Runtime Error] Cannot initialize console");
    console->initConsole();
}

// Main loop
void Application::mainLoop() {
    // Declaration of some variables for fps display
    double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    double fpsTimer = lastTime;
    double fps = 0.0;
    int nbFrames = 0;

    while (!glfwWindowShouldClose(window->get())) {
        // Main rendering (the simulation grid)
        renderer->render();

        // Console rendering on top of the grid
        console->draw();
        
        glfwSwapBuffers(window->get());
        
        glfwPollEvents();
        
        // Solving the next simulation step if not paused
        if (!grid->pause) {
            grid->step();
        }

        // fps counter and display
        if (cfg->showfps) {
            currentTime = glfwGetTime();
            nbFrames++;
            if (currentTime - fpsTimer >= 0.25) {
                fps = nbFrames / (currentTime - fpsTimer);
                title = "GOL - FPS: " + std::format("{:.2f}", fps);
                glfwSetWindowTitle(window->get(), title.c_str());
                nbFrames = 0;
                fpsTimer = currentTime;
            }
            lastTime = currentTime;
        }
    }

    glfwTerminate();
}