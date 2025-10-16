#include "app.hpp"
#include "GLFW/glfw3.h"

#include <windows.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <format>

Application::Application() {

}

Application::~Application() {
    cleanup();
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

void Application::loadConfig() {
    cfg = std::make_unique<Config>();
    cfg->initConfig("config.jsonc");
    cfg->printAllParams();
    if (cfg->width < 120) {
        std::cout << "Warning : minimum allowed width is 120. Moved back to this value.\n";
        cfg->width = 120;
    }
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

int Application::initWindow() {
    window = std::make_unique<Window>(cfg->width, cfg->height, title);
    cfg->window = window->get();
    window->makeContextCurrent();
    window->setUserPointer(this);
    set_window_icon_from_resource(window->get());
    
    glfwSetKeyCallback(window->get(), key_callback);
    glfwSetCharCallback(window->get(), char_callback);
    
    if (cfg->vsync || (cfg->freeze_at_start && !cfg->vsync)) {
        glfwSwapInterval(1);
    } else { 
        glfwSwapInterval(0);
    }

    return 0;
}

void Application::initConsole() {
    console = std::make_unique<Console>();
    luaengine = std::make_unique<LuaEngine>();
    console->init();
    luaengine->init();
    luaengine->bindConfig(cfg.get());
    luaengine->bindGrid(grid.get());
    console->lua = luaengine.get();
    luaengine->registerPrintRedirect([&](const std::string& s){ console->log(s); });
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

void Application::initRender() {
    renderer = std::make_unique<Renderer>(grid.get(), cfg.get());
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

void Application::cleanup() {

}