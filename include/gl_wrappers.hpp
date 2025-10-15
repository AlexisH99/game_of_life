#pragma once

#include <glad/gl.h>

class GLVertexBuffer {
    public:
        GLVertexBuffer();
        ~GLVertexBuffer();

        void bind() const;
        void unbind() const;

        GLVertexBuffer(const GLVertexBuffer&) = delete;
        GLVertexBuffer& operator=(const GLVertexBuffer&) = delete;

        GLVertexBuffer(GLVertexBuffer&& other) noexcept;
        GLVertexBuffer& operator=(GLVertexBuffer&& other) noexcept;

    private:
        GLuint id = 0;
};

class GLBuffer {
    public:
        GLBuffer(GLenum target);
        ~GLBuffer();

        void bind() const;
        void unbind() const;
        void set_data(GLsizeiptr size, const void* data, GLenum usage);

        GLBuffer(const GLBuffer&) = delete;
        GLBuffer& operator=(const GLBuffer&) = delete;
        
        GLBuffer(GLBuffer&& other) noexcept;
        GLBuffer& operator=(GLBuffer&& other) noexcept;

    private:
        GLuint id = 0;
        GLenum target;
};

class GLTexture {
    public:
        GLTexture(GLenum target);
        ~GLTexture();

        void bind(GLuint unit) const;
        void set_parameters() const;
        void allocate(GLint internalFormat, GLsizei width, GLsizei height,
            GLenum format, GLenum type, const void* data) const;

        GLTexture(const GLTexture&) = delete;
        GLTexture& operator=(const GLTexture&) = delete;

        GLTexture(GLTexture&& other) noexcept;
        GLTexture& operator=(GLTexture&& other) noexcept;
    
    private:
        GLuint id = 0;
        GLenum target;
};

class GLProgram {
    public:
        GLProgram() = default;
        GLProgram(const char* vertSrc, const char* fragSrc);
        ~GLProgram();

        void loadFromSource(const char* vertSrc, const char* fragSrc);
        void use() const;

        GLProgram(const GLProgram&) = delete;
        GLProgram& operator=(const GLProgram&) = delete;

        GLProgram(GLProgram&& other) noexcept;
        GLProgram& operator=(GLProgram&& other) noexcept;

    private:
        static GLuint compile(GLenum type, const char* src);

        GLuint id = 0;
};