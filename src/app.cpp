#include "app.hpp"
#include "config.hpp"
#include <cstdlib>
#include <random>

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

Application::Application()
    : rng(std::random_device{}()), dist(0, 1) {}

Application::~Application() {
    cleanup();
}

void Application::generateRandomTexture(int width, int height) {
    current.resize(width * height);
    next.resize(width * height);
    for (int i = 0; i < width * height; i++) {
        current[i] = dist(rng) ? 255 : 0;
    }
}

void Application::generateCheckerTexture(int width, int height) {
    current.resize(width * height);
    next.resize(width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            // Case blanche si (x+y) pair, noire sinon
            current[idx] = ((x + y) % 2 == 0) ? 255 : 0;
        }
    }
}

void Application::updateGameOfLife(int width, int height) {
    auto getCell = [&](int x, int y) -> int {
        if (x < 0 || y < 0 || x >= width || y >= height) return 0; // bords = morts
        return current[y * width + x] > 0; // 0 ou 1
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int alive = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    alive += getCell(x + dx, y + dy);
                }
            }
            int idx = y * width + x;
            if (current[idx]) {
                next[idx] = (alive == 2 || alive == 3) ? 255 : 0;
            } else {
                next[idx] = (alive == 3) ? 255 : 0;
            }
        }
    }

    current.swap(next);
}

void Application::run() {
    initWindow();
    initGlad();
    initShaders();
    initRender();
    mainLoop();
}

int Application::initWindow() {
    if (!glfwInit()) {
        std::cerr << "Erreur init GLFW !" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "GOL", nullptr, nullptr);
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
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Erreur init GLAD !" << std::endl;
        return -1;
    }

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return 0;
}

void Application::initShaders() {
    vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;

        out vec2 TexCoord;

        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec2 TexCoord;
        uniform sampler2D ourTexture;

        void main() {
            float val = texture(ourTexture, TexCoord).r; // lecture canal rouge
            FragColor = vec4(val, val, val, 1.0);        // noir/blanc
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

    generateRandomTexture(GRIDX, GRIDY);
    //generateCheckerTexture(GRIDX, GRIDY);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Réglages de filtrage et wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Allocation + upload initial
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, GRIDX, GRIDY, 0,
                GL_RED, GL_UNSIGNED_BYTE, current.data());
}

void Application::mainLoop() {
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    while (!glfwWindowShouldClose(window)) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRIDX, GRIDY,
                        GL_RED, GL_UNSIGNED_BYTE, current.data());

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Dessin
        glUseProgram(shaderProgram);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Swap buffers et poll events
        glfwSwapBuffers(window);
        glfwPollEvents();

        updateGameOfLife(GRIDX, GRIDY);

        double currentTime = glfwGetTime();
        nbFrames++;

        if (currentTime - lastTime >= 1.0) { // 1 seconde écoulée
            std::cout << "FPS: " << nbFrames << std::endl;
            nbFrames = 0;
            lastTime = glfwGetTime();
        }
    }
}

void Application::cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
}