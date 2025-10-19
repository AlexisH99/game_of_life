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

void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        glViewport(0, 0, width, height);
        app->cfg->width = width;
        app->cfg->height = height;
    }
}

void Application::set_window_icon_from_resource(GLFWwindow* window) {
    HICON hIcon = (HICON)LoadImage(
        GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDI_APP_ICON),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTSIZE
    );

    if (!hIcon) {
        std::cerr << "Cannot load resource from executable" << std::endl;
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

void Application::key_callback(GLFWwindow* window, int key, [[maybe_unused]]int scancode, [[maybe_unused]]int action, [[maybe_unused]]int mods)
{   
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    glfwMakeContextCurrent(window);
    if (app) {
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
        if (app->grid->pause && key == GLFW_KEY_RIGHT && action == GLFW_PRESS && !app->console->visible) {
            app->grid->step();
        }
        if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
            app->console->visible = !app->console->visible;
        }

        if (app->console->visible) {
            app->console->handleInput(app->window->get(), key, action);
        }
    }
}

void Application::char_callback(GLFWwindow* window, unsigned int codepoint){
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    app->console->handleChar(codepoint);
}

void Application::scroll_callback(GLFWwindow* window, [[maybe_unused]]double xoffset, double yoffset) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app || !app->renderer) return;

    Renderer* r = app->renderer.get();
    Config* c = app->cfg.get();

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    mouseY = c->height - mouseY;

    float gridX_before = r->camX + (mouseX / c->width)  * (c->gridx / r->zoom);
    float gridY_before = r->camY + (mouseY / c->height) * (c->gridy / r->zoom);

    float prevZoom = r->zoom;
    r->zoom *= pow(r->zoomSpeed, yoffset);

    float maxZoom = 100.0f * std::max(
        (float)c->gridx / (float)c->width,
        (float)c->gridy / (float)c->height
    );
    r->zoom = std::clamp(r->zoom, r->minZoom, maxZoom);

    float gridX_after = r->camX + (mouseX / c->width)  * (c->gridx / r->zoom);
    float gridY_after = r->camY + (mouseY / c->height) * (c->gridy / r->zoom);

    r->camX += (gridX_before - gridX_after);
    r->camY += (gridY_before - gridY_after);

    float visibleWidth  = c->gridx / r->zoom;
    float visibleHeight = c->gridy / r->zoom;

    r->camX = std::clamp(r->camX, 0.0f, c->gridx - visibleWidth);
    r->camY = std::clamp(r->camY, 0.0f, c->gridy - visibleHeight);

    std::cout << r->camX << " " << r->camY << "\n";
    std::cout << "zoom " << r->zoom << "\n";
}

void Application::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    static double lastX = xpos, lastY = ypos;
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app || !app->renderer) return;

    Renderer* r = app->renderer.get();
    Config* c = app->cfg.get();

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double dx = xpos - lastX;
        double dy = ypos - lastY;

        float cellPerPixelX = (c->gridx / r->zoom) / c->width;
        float cellPerPixelY = (c->gridy / r->zoom) / c->height;

        r->camX -= dx * cellPerPixelX;
        r->camY += dy * cellPerPixelY;

        float visibleWidth  = c->gridx / r->zoom;
        float visibleHeight = c->gridy / r->zoom;

        r->camX = std::clamp(r->camX, 0.0f, c->gridx - visibleWidth);
        r->camY = std::clamp(r->camY, 0.0f, c->gridy - visibleHeight);

        std::cout << r->camX << " " << r->camY << "\n";
    }

    lastX = xpos;
    lastY = ypos;
}


void Application::loadConfig() {
    cfg = std::make_unique<Config>();
    cfg->initConfig("config.jsonc");
    cfg->printAllParams();
    if (cfg->width < 120) {
        std::cout << "Warning : minimum allowed width is 120. Moved back to this value.\n";
        cfg->width = 120;
    }
}

int Application::initWindow() {
    window = std::make_unique<Window>(cfg->width, cfg->height, title);
    cfg->window = window->get();
    window->makeContextCurrent();
    window->setUserPointer(this);
    set_window_icon_from_resource(window->get());
    
    glfwSetKeyCallback(window->get(), key_callback);
    glfwSetCharCallback(window->get(), char_callback);
    glfwSetScrollCallback(window->get(), scroll_callback);
    glfwSetCursorPosCallback(window->get(), cursor_position_callback);
    
    if (cfg->vsync || (cfg->freeze_at_start && !cfg->vsync)) {
        glfwSwapInterval(1);
    } else { 
        glfwSwapInterval(0);
    }

    return 0;
}

int Application::initGlad() {
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Error GLAD init" << std::endl;
        return -1;
    }

    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL version: " << version << std::endl;
    if (!GLAD_GL_VERSION_3_3) throw std::runtime_error("OpenGL 3.3 or higher required.");

    GLint supported = 0;
    glGetInternalformativ(GL_TEXTURE_2D, GL_RG32UI, GL_INTERNALFORMAT_SUPPORTED, 1, &supported);
    if (!supported) throw std::runtime_error("GL_RG32UI not supported on this GPU.");

    glfwGetFramebufferSize(window->get(), &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glfwSetFramebufferSizeCallback(window->get(), framebuffer_size_callback);

    return 0;
}

void Application::initGrid() {
    grid = std::make_unique<Grid>();
    grid->cfg = cfg.get();
    grid->pause = cfg->freeze_at_start;
    grid->initSize();
    grid->initMask();
    if (cfg->checker == true) {
        grid->initCheckerGrid();
    } else {
        grid->initRandomGrid();
    }
}

void Application::initRender() {
    renderer = std::make_unique<Renderer>(grid.get(), cfg.get());
}

void Application::initConsole() {
    console = std::make_unique<Console>(cfg.get(), window.get(), grid.get(), renderer.get());
    console->init();
}

void Application::mainLoop() {
    double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    double fpsTimer = lastTime;
    double fps = 0.0;
    int nbFrames = 0;

    while (!glfwWindowShouldClose(window->get())) {
        renderer->render();

        console->draw(window->get());
        
        glfwSwapBuffers(window->get());
        
        glfwPollEvents();
        
        if (!grid->pause) {
            grid->step();
        }

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
}