#include "shader.hpp"
#include "app.hpp"
#include <string>
#include <iostream>

Shaders::Shaders() {

}

Shaders::~Shaders() {

}

void Shaders::init(GLFWwindow* window) {
    glfwMakeContextCurrent(window);
    gladLoadGL(glfwGetProcAddress);
    mainVertShader = compileShader(GL_VERTEX_SHADER, mainVert);
    mainFragShader = compileShader(GL_FRAGMENT_SHADER, mainFrag);
    mainShader.program = linkProgram(mainVertShader, mainFragShader);
    glDeleteShader(mainVertShader);
    glDeleteShader(mainFragShader);

    consoleVertShader = compileShader(GL_VERTEX_SHADER, consoleVert);
    consoleFragShader = compileShader(GL_FRAGMENT_SHADER, consoleFrag);
    consoleShader.program = linkProgram(consoleVertShader, consoleFragShader);
    glDeleteShader(consoleVertShader);
    glDeleteShader(consoleFragShader);
}

unsigned int Shaders::compileShader(GLenum type, const char* src) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        std::cerr << "Shader compile error:\n" << info << std::endl;
    }

    return shader;
}

unsigned int Shaders::linkProgram(unsigned int vert, unsigned int frag) {
    unsigned int program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info[512];
        glGetProgramInfoLog(program, 512, nullptr, info);
        std::cerr << "Program link error:\n" << info << std::endl;
    }

    return program;
}