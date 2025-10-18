# pragma once

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

    uniform usamplerBuffer packedGrid;  // plus de layout(binding)
    uniform int leftpad;
    uniform vec2 windowSize;
    uniform vec2 gridSize;
    uniform int words_per_row;
    uniform float zoom;
    uniform vec2 camera;

    out vec4 FragColor;

    void main() {
        vec2 frag = gl_FragCoord.xy - vec2(0.5);

        float gx_f = camera.x + frag.x * (gridSize.x / windowSize.x) / zoom;
        float gy_f = camera.y + frag.y * (gridSize.y / windowSize.y) / zoom;

        int gx = int(gx_f);
        int gy = int(gy_f);

        if (gx < 0 || gx >= int(gridSize.x) || gy < 0 || gy >= int(gridSize.y))
            discard;

        int y = gy + 1;
        int x = gx + leftpad;

        int word_index = x / 64;
        int bit_index  = x % 64;

        // === Index linéaire dans le buffer ===
        int linearIndex = y * words_per_row + word_index;

        // Chaque texel contient deux uint32 (low/high)
        uvec2 word = texelFetch(packedGrid, linearIndex).rg;

        // === Extraction du bit vivant ===
        uint alive = (bit_index < 32)
            ? ((word.r >> uint(bit_index)) & 1u)
            : ((word.g >> uint(bit_index - 32)) & 1u);

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
