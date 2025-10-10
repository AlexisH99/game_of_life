#include "app.hpp"

#include <iostream>
#include <cstdlib>
#include <random>
#include <format>

void framebuffer_size_callback([[maybe_unused]]GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

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

void Application::loadConfig() {
    cfg.initConfig("config.json");
    cfg.printAllParams();
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
    glfwSwapInterval(0);

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

        uniform int gridX;
        uniform int gridY;
        uniform int leftpad;
        uniform int words_per_row;

        out vec4 FragColor;

        void main() {
            ivec2 frag = ivec2(gl_FragCoord.xy);

            // Ignorer padding vertical
            if (frag.y >= gridY) discard;

            // Conversion vers indices packés
            int y = frag.y + 1;             // saute la ligne de padding du haut
            int x = frag.x + leftpad;       // saute le padding horizontal gauche

            int word_index = x / 64;
            int bit_index  = x % 64;

            // Lire deux uint32 (mot bas + mot haut)
            uvec2 wordpair = texelFetch(packedGrid, ivec2(word_index, y), 0).rg;
            uint low  = wordpair.r;   // bits 0–31
            uint high = wordpair.g;   // bits 32–63

            uint alive;
            if (bit_index < 32) {
                alive = (low >> bit_index) & 1u;
            } else {
                alive = (high >> (bit_index - 32)) & 1u;
            }

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

    // Attribut position (vec2)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Réglages de filtrage et wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Allocation + upload initial
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, cfg.gridx, cfg.gridy, 0,
    //            GL_RED, GL_UNSIGNED_BYTE, grid.getUnpacked().data());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32UI,
            grid.words_per_row, grid.rows, 0,
            GL_RG_INTEGER, GL_UNSIGNED_INT,
            reinterpret_cast<const uint32_t*>(grid.getGrid().data()));
}

void Application::mainLoop() {
    double lastTime = glfwGetTime();
    double fpsTimer = lastTime;
    double fps = 0.0;
    int nbFrames = 0;

    while (!glfwWindowShouldClose(window)) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, grid.words_per_row, grid.rows,
                        GL_RG_INTEGER, GL_UNSIGNED_INT, reinterpret_cast<const uint32_t*>(grid.getGrid().data()));

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Dessin
        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "gridX"), grid.gridX);
        glUniform1i(glGetUniformLocation(shaderProgram, "gridY"), grid.gridY);
        glUniform1i(glGetUniformLocation(shaderProgram, "leftpad"), grid.leftpad);
        glUniform1i(glGetUniformLocation(shaderProgram, "words_per_row"), grid.words_per_row);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Swap buffers et poll events
        glfwSwapBuffers(window);
        glfwPollEvents();

        //grid.step();

        double currentTime = glfwGetTime();
        nbFrames++;

        if (currentTime - fpsTimer >= 0.25) {
            fps = nbFrames / (currentTime - fpsTimer);
            std::cout << "FPS: " << fps << std::endl;
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