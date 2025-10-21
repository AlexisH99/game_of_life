#include "gl_wrappers.hpp"

#include <utility>
#include <stdexcept>
#include <string>

GLVertexBuffer::GLVertexBuffer() {
    glGenVertexArrays(1, &id);
}

GLVertexBuffer::~GLVertexBuffer() {
    if (id) glDeleteVertexArrays(1, &id);
}

void GLVertexBuffer::bind() const {
    glBindVertexArray(id);
}

void GLVertexBuffer::unbind() const {
    glBindVertexArray(0);
}

GLVertexBuffer::GLVertexBuffer(GLVertexBuffer&& other) noexcept
    : id(other.id)
{
    other.id = 0;
}

GLVertexBuffer& GLVertexBuffer::operator=(GLVertexBuffer&& other) noexcept {
    std::swap(id, other.id);
    return *this;
}

GLBuffer::GLBuffer([[maybe_unused]]GLenum tgt)
    : target(tgt)
{
    glGenBuffers(1, &id);
}

GLBuffer::~GLBuffer() {
    if (id) glDeleteBuffers(1, &id);
}

void GLBuffer::bind() const {
    glBindBuffer(target, id);
}

void GLBuffer::unbind() const {
    glBindBuffer(target, 0);
}

void GLBuffer::set_data(GLsizeiptr size, const void* data, GLenum usage = GL_STATIC_DRAW) {
    glBindBuffer(target, id);
    glBufferData(target, size, data, usage);
}

GLBuffer::GLBuffer(GLBuffer&& other) noexcept
    : id(other.id), target(other.target)
{
    other.id = 0;
}

GLBuffer& GLBuffer::operator=(GLBuffer&& other) noexcept {
    std::swap(id, other.id);
    std::swap(target, other.target);
    return *this;
}

GLTextureBuffer::GLTextureBuffer() {
    glGenTextures(1, &texID);
    glGenBuffers(1, &bufID);
}

GLTextureBuffer::~GLTextureBuffer() {
    if (texID) glDeleteTextures(1, &texID);
    if (bufID) glDeleteBuffers(1, &bufID);
}

void GLTextureBuffer::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_BUFFER, texID);
}

void GLTextureBuffer::allocate(GLenum internalFormat, GLsizeiptr sizeBytes, const void* data, GLenum usage) {
    glBindBuffer(GL_TEXTURE_BUFFER, bufID);
    glBufferData(GL_TEXTURE_BUFFER, sizeBytes, data, usage);

    glBindTexture(GL_TEXTURE_BUFFER, texID);
    glTexBuffer(GL_TEXTURE_BUFFER, internalFormat, bufID);
}

void GLTextureBuffer::update(GLsizeiptr sizeBytes, const void* data, GLintptr offset) {
    glBindBuffer(GL_TEXTURE_BUFFER, bufID);
    glBufferSubData(GL_TEXTURE_BUFFER, offset, sizeBytes, data);
}

GLTextureBuffer::GLTextureBuffer(GLTextureBuffer&& other) noexcept
    : texID(other.texID), bufID(other.bufID)
{
    other.texID = 0;
    other.bufID = 0;
}

GLTextureBuffer& GLTextureBuffer::operator=(GLTextureBuffer&& other) noexcept {
    std::swap(texID, other.texID);
    std::swap(bufID, other.bufID);
    return *this;
}

GLProgram::GLProgram(const char* vertSrc, const char* fragSrc) {
    loadFromSource(vertSrc, fragSrc);
}

GLProgram::~GLProgram() {
    if (id) glDeleteProgram(id);
}

void GLProgram::loadFromSource(const char* vertSrc, const char* fragSrc) {
    GLuint vert = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint frag = compile(GL_FRAGMENT_SHADER, fragSrc);

    id = glCreateProgram();
    glAttachShader(id, vert);
    glAttachShader(id, frag);
    glLinkProgram(id);

    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(id, 512, nullptr, log);
        throw std::runtime_error(std::string("Program link error: ") + log);
    }
}

void GLProgram::use() const {
    glUseProgram(id);
}

GLuint GLProgram::get() const {
    return id;
}

GLuint GLProgram::compile(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        throw std::runtime_error(std::string("Shader compile error: ") + log);
    }
    return shader;
}

GLProgram::GLProgram(GLProgram&& other) noexcept
    : id(other.id)
{
    other.id = 0;
}

GLProgram& GLProgram::operator=(GLProgram&& other) noexcept {
    std::swap(id, other.id);
    return *this;
}