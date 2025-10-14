#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>

class Shaders {
    public:
        Shaders();
        ~Shaders();

        void init();

        unsigned int compileShader(GLenum type, const char* src);
        unsigned int linkProgram(unsigned int vert, unsigned int frag);

        struct ShaderProgram {
            GLuint program = 0;
            ~ShaderProgram() { if (program) glDeleteProgram(program); }
        };
        
        ShaderProgram mainShader, consoleShader;
        
    private:
        unsigned int mainVertShader = 0;
        unsigned int mainFragShader = 0;
        unsigned int consoleVertShader = 0;
        unsigned int consoleFragShader = 0;

        static constexpr const char* mainVert = R"(
            #version 330 core

            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;

            out vec2 TexCoord;

            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
                TexCoord = aTexCoord;
            }
        )";
        static constexpr const char* mainFrag = R"(
            #version 330 core

            uniform usampler2D packedGrid;  // plus de layout(binding)
            uniform int leftpad;
            uniform vec2 windowSize;
            uniform vec2 gridSize;

            out vec4 FragColor;

            void main() {
                vec2 frag = gl_FragCoord.xy - vec2(0.5);

                int gx = int(frag.x * (gridSize.x / windowSize.x));
                int gy = int(frag.y * (gridSize.y / windowSize.y));

                if (gx < 0 || gx >= int(gridSize.x) || gy < 0 || gy >= int(gridSize.y))
                    discard;

                int y = gy + 1;
                int x = gx + leftpad;

                int word_index = x / 64;
                int bit_index  = x % 64;

                uvec4 texel = texelFetch(packedGrid, ivec2(word_index, y), 0);
                uint low  = texel.r;
                uint high = texel.g;

                uint alive = (bit_index < 32)
                    ? ((low >> bit_index) & 1u)
                    : ((high >> (bit_index - 32)) & 1u);

                float val = float(alive);
                FragColor = vec4(val, val, val, 1.0);
            }
        )";
        static constexpr const char* consoleVert = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            uniform vec2 uScreen;
            void main() {
                vec2 ndc = (aPos / uScreen) * 2.0 - 1.0;
                ndc.y = -ndc.y; // Origine en haut
                gl_Position = vec4(ndc, 0.0, 1.0);
                gl_PointSize = 1.0;
            }
        )";
        static constexpr const char* consoleFrag = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 uColor;
            void main() {
                FragColor = uColor;
            }
        )";
};