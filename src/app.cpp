#include "app.hpp"

#include <iostream>
#include <cstdlib>
#include <format>

void checkCompileErrors(unsigned int shader, std::string type) {
    int success;
    char infoLog[512];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Erreur compilation shader (" << type << "): " << infoLog << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Erreur link program: " << infoLog << std::endl;
        }
    }
}

Application::Application() {

}

Application::~Application() {
    cleanup();
}

void Application::run() {
    loadConfig();
    initGrid();
    initWindow();
    initGlad();
    initShaders();
    initRender();
    mainLoop();
}

void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        glViewport(0, 0, width, height);
        app->cfg.width = width;
        app->cfg.height = height;
    }
}

void Application::key_callback(GLFWwindow* window, int key, [[maybe_unused]]int scancode, [[maybe_unused]]int action, [[maybe_unused]]int mods)
{   
    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) {
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            app->pause = !app->pause;
            if (!app->cfg.vsync) {
                if (app->pause) {
                    glfwSwapInterval(1);
                } else {
                    glfwSwapInterval(0);
                }
            }
        }
        if (app->pause && key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
            app->grid.step();
        }
    }
}

void Application::loadConfig() {
    cfg.initConfig("config.json");
    cfg.printAllParams();
    pause = cfg.freeze_at_start;
}

void Application::initGrid() {
    grid.initSize(cfg.gridx, cfg.gridy, 1);
    grid.initMask();
    if (cfg.checker == true) {
        grid.initCheckerGrid();
    } else {
        grid.initRandomGrid();
    }
}

int Application::initWindow() {
    if (!glfwInit()) {
        std::cerr << "Erreur init GLFW !" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(cfg.width, cfg.height, title.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Erreur création fenêtre !" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, key_callback);
    if (cfg.vsync || (pause & !cfg.vsync)) {
        glfwSwapInterval(1);
    } else { 
        glfwSwapInterval(0);
    }

    return 0;
}

int Application::initGlad() {
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Erreur init GLAD !" << std::endl;
        return -1;
    }

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return 0;
}

void Application::initShaders() {
    vertexShaderSource = R"(
        #version 430 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;

        out vec2 TexCoord;

        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    fragmentShaderSource = R"(
        #version 430 core
        layout(binding = 0) uniform usampler2D packedGrid;

        uniform int leftpad;
        uniform vec2 windowSize;
        uniform vec2 gridSize;

        out vec4 FragColor;

        void main() {
            vec2 frag = (gl_FragCoord.xy - vec2(0.5));

            int gx = int(frag.x * (gridSize.x / windowSize.x));
            int gy = int(frag.y * (gridSize.y / windowSize.y));

            int y = gy + 1;
            int x = gx + leftpad;

            if (gx < 0 || gx >= gridSize.x || gy < 0 || gy >= gridSize.y) {
                discard;
            }

            int word_index = x / 64;
            int bit_index  = x % 64;

            uvec2 wordpair = texelFetch(packedGrid, ivec2(word_index, y), 0).rg;
            uint low  = wordpair.r;
            uint high = wordpair.g;

            uint alive = (bit_index < 32)
                    ? ((low >> bit_index) & 1u)
                    : ((high >> (bit_index - 32)) & 1u);

            float val = float(alive);
            FragColor = vec4(val, val, val, 1.0);
        }
    )";

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    shaderProgram = glCreateProgram();

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    checkCompileErrors(vertexShader, "VERTEX");

    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    checkCompileErrors(fragmentShader, "FRAGMENT");

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    checkCompileErrors(shaderProgram, "PROGRAM");

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Application::initRender() {
    float vertices[] = {
        // pos       // texcoords
        -1.0f,  1.0f,  0.0f, 1.0f, // haut gauche
        -1.0f, -1.0f,  0.0f, 0.0f, // bas gauche
        1.0f, -1.0f,  1.0f, 0.0f, // bas droit

        -1.0f,  1.0f,  0.0f, 1.0f, // haut gauche
        1.0f, -1.0f,  1.0f, 0.0f, // bas droit
        1.0f,  1.0f,  1.0f, 1.0f  // haut droit
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI,
            grid.words_per_row, grid.rows, 0,
            GL_RG_INTEGER, GL_UNSIGNED_INT,
            grid.getGrid32Ptr());
}

void Application::mainLoop() {
    double lastTime = glfwGetTime();
    double currentTime = glfwGetTime();
    double fpsTimer = lastTime;
    double fps = 0.0;
    int nbFrames = 0;

    while (!glfwWindowShouldClose(window)) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, grid.words_per_row, grid.rows,
                        GL_RG_INTEGER, GL_UNSIGNED_INT, grid.getGrid32Ptr());

        glClearColor(1.0f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "leftpad"), grid.leftpad);
        glUniform2f(glGetUniformLocation(shaderProgram, "windowSize"), cfg.width, cfg.height);
        glUniform2f(glGetUniformLocation(shaderProgram, "gridSize"), cfg.gridx, cfg.gridy);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        if (!pause) {
            grid.step();
        }

        currentTime = glfwGetTime();
        nbFrames++;

        if (currentTime - fpsTimer >= 0.25) {
            fps = nbFrames / (currentTime - fpsTimer);
            title = "GOL - FPS: " + std::format("{:.2f}", fps);
            glfwSetWindowTitle(window, title.c_str());

            nbFrames = 0;
            fpsTimer = currentTime;
        }

        lastTime = currentTime;
    }
}

void Application::cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
}