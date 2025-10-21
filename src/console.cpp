#include "console.hpp"
#include "shaders_sources.hpp"
#include "font8x8_basic.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <thread>
#include <sstream>
#include <charconv>
#include <functional>
#include <iostream>
#include <algorithm>

Console::Console(Config* cfg, Window* win, Grid* grid, Renderer* renderer)
    : cfg(cfg), win(win), grid(grid), renderer(renderer)
{

}

Console::~Console() {

}

void Console::init() {
    vao = std::make_unique<GLVertexBuffer>();
    vbo = std::make_unique<GLBuffer>(GL_ARRAY_BUFFER);
    shaders = std::make_unique<GLProgram>(consoleVert, consoleFrag);

    log("Available commands:");
    log("  start / stop / regen");
    log("  step <n_steps> <delay>");
    log("  get <globalProperty>");
    log("  set <globalProperty> [values]");
    log("Available globalProperties:");
    log("  windowSize | gridSize | ruleSet | seed | dist");
}

template<typename T>
std::optional<T> Console::from_string(const std::string& s) {
    static_assert(std::is_arithmetic_v<T>, "T doit être un type numérique");

    T value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

    if (ec == std::errc()) return value;
    return std::nullopt;
}

void Console::log(const std::string& s) {
    lines.push_back(s);
    if (lines.size() > 1000) lines.erase(lines.begin());
}

void Console::execute(const std::string& command) {
    commandIndex = 0;
    commandHistory.push_front(command);
    if (commandHistory.size() > 100) commandHistory.erase(commandHistory.begin());
    log("> " + command);

    // === 1. Tokenize ===
    std::istringstream iss(command);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token)
        tokens.push_back(token);

    if (tokens.empty()) return;
    const std::string& cmd = tokens[0];

    // === 2. Commandes simples (start, stop, regen, step...) ===
    static const std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> baseCommands = {
        { "start", [&](const auto&) { command_start(); } },
        { "stop",  [&](const auto&) { command_stop(); } },
        { "regen", [&](const auto&) {
            command_regen();
            if (cfg->randomSeed) {
                log("Set seed to: " + std::to_string(grid->gridSeed));
            } else {
                log("Seed: " + std::to_string(grid->gridSeed));
            }
        } },

        { "step", [&](const auto& args) {
            if (args.size() == 1) {
                command_step();
            } else if (args.size() == 2) {
                auto n = from_string<int>(args[1]);
                if (!n) log("Usage: step <int>");
                else command_step(*n);
            } else if (args.size() == 3) {
                auto n = from_string<int>(args[1]);
                auto d = from_string<float>(args[2]);
                if (!n || !d) log("Usage: step <int> <float>");
                else command_step(*n, *d);
            } else log("Usage: step <int> [float]");
        }},

        { "help", [&](const auto&) {
            log("Available commands:");
            log("  start / stop / regen");
            log("  step <n_steps> <delay>");
            log("  get <globalProperty>");
            log("  set <globalProperty> [values]");
            log("Available globalProperties:");
            log("  windowSize | gridSize | ruleSet | seed | dist");
        }},
    };

    // === 3. Commande "get" ===
    if (cmd == "get") {
        if (tokens.size() != 2) {
            log("Usage: get <windowSize|gridSize>");
            return;
        }
        const auto& sub = tokens[1];
        if (sub == "windowSize") {
            getWindowSize();
        } else if (sub == "gridSize") {
            getGridSize();
        } else {
            log("Unknown parameter: " + sub);
        }
        return;
    }

    // === 4. Commande "set" ===
    if (cmd == "set") {
        if (tokens.size() < 2) {
            log("Usage: set <windowSize|gridSize> <values>");
            return;
        }

        const auto& sub = tokens[1];

        // ---- set windowSize <int|.> <int|.> ----
        if (sub == "windowSize") {
            if (tokens.size() != 4) {
                log("Usage: set windowSize <int|.> <int|.>");
                return;
            }
            auto w_str = tokens[2], h_str = tokens[3];

            if (w_str == "." && h_str == ".") {
                log("Unchanged window size");
                return;
            }

            auto w = (w_str == ".") ? std::optional<int>() : from_string<int>(w_str);
            auto h = (h_str == ".") ? std::optional<int>() : from_string<int>(h_str);

            if ((w_str != "." && !w) || (h_str != "." && !h)) {
                log("Error: invalid arguments");
                log("Usage: set windowSize <int|.> <int|.>");
                return;
            }

            setWindowSize(w.value_or(cfg->width), h.value_or(cfg->height));
            return;
        }

        // ---- set gridSize <int> <int> ----
        if (sub == "gridSize") {
            if (tokens.size() != 4) {
                log("Usage: set gridSize <int> <int>");
                return;
            }

            auto gx = from_string<int>(tokens[2]);
            auto gy = from_string<int>(tokens[3]);

            if (!gx || !gy) {
                log("Error: invalid arguments");
                log("Usage: set gridSize <int> <int>");
                return;
            }

            setGridSize(*gx, *gy);
            return;
        }

        if (sub == "ruleSet") {
            if (tokens.size() != 3) {
                log("Usage: set ruleSet <str>");
                return;
            }
            setRuleset(tokens[2]);
            return;
        }

        if (sub == "seed") {
            if (tokens.size() != 3) {
                log("Usage: set ruleSet <str>/<int>");
                return;
            }
            auto seed = from_string<int>(tokens[2]);
            if (!seed) {
                if (tokens[2] == "rnd") {
                    setSeed(true, 0);
                    log("Random seed");
                } else {
                    log("Error: invalid argument '" + tokens[2] + "'");
                    log("Usage: set ruleSet <str>/<int>");
                }
            } else {
                setSeed(false, *seed);
                log("Set seed to: " + std::to_string(grid->gridSeed));
            }
            return;
        }

        if (sub == "dist") {
            if (tokens.size() != 3 && tokens.size() != 4) {
                log("Usage: set ruleSet <str> <float>");
                return;
            }
            auto distType = tokens[2];
            auto [ok, msg] = cfg->parseDistType(distType);
            if (tokens.size() == 3) {
                setDistrib(distType);
            }
            if (tokens.size() == 4) {
                auto density = from_string<float>(tokens[3]);
                setDistrib(distType, *density);
            }
            return;
        }

        log("Unknown parameter: " + sub);
        return;
    }

    // === 5. Commandes de base ===
    auto it = baseCommands.find(cmd);
    if (it != baseCommands.end()) {
        it->second(tokens);
        return;
    }

    // === 6. Commande inconnue ===
    log("Unknown command: " + cmd);
    log("Type 'help' for available commands.");
}

void Console::draw(GLFWwindow* window) {
    if (!visible) return;
    shaders->use();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    cWidth = (float)fbWidth;
    cHeight = (float)(0.3 * fbHeight);

    // === fond semi-transparent ===
    float bgVerts[12] = {
        0, 0,  cWidth, 0,  cWidth, cHeight,
        0, 0,  cWidth, cHeight, 0, cHeight
    };
    vao->bind();
    vbo->bind();
    vbo->set_data(sizeof(bgVerts), bgVerts, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glUniform2f(glGetUniformLocation(shaders->get(), "uScreen"), (float)fbWidth, (float)fbHeight);
    glUniform4f(glGetUniformLocation(shaders->get(), "uColor"), 0.0f, 0.0f, 0.15f, 0.75f);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // === texte vert ===
    pts.clear();
    int lineHeight = 10;
    maxVisibleLines = (int)(cHeight / lineHeight) - 3;
    if (maxVisibleLines < 1) maxVisibleLines = 1;

    int y = 10;
    int end = std::min((int)lines.size(), lineOffset + maxVisibleLines);
    for (int i = lineOffset; i < end; i++) {
        appendText(pts, 10, y, lines[i]);
        y += lineHeight;
    }
    appendText(pts, 10, cHeight - lineHeight, "> " + input + "_");

    vbo->set_data(pts.size() * sizeof(float), pts.data(), GL_STREAM_DRAW);
    glUniform4f(glGetUniformLocation(shaders->get(), "uColor"), 0.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_POINTS, 0, pts.size()/2);

    glDisable(GL_BLEND);
}

void Console::handleInput([[maybe_unused]]GLFWwindow* win, int key, int action) {
    if (!visible) return;
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_ENTER) {
            execute(input);
            input.clear();
            lineOffset = std::max(0, (int)lines.size() - maxVisibleLines);
        } else if (key == GLFW_KEY_BACKSPACE && !input.empty()) {
            input.pop_back();
        } else if (key == GLFW_KEY_UP) {
            commandIndex += 1;
            commandIndex = std::clamp(commandIndex, 0, (int)commandHistory.size());
            if (commandHistory.size() == 0) return;
            input.clear();
            input = commandHistory[commandIndex-1];
        } else if (key == GLFW_KEY_DOWN) {
            commandIndex -= 1;
            commandIndex = std::clamp(commandIndex, 0, (int)commandHistory.size());
            if (commandHistory.size() == 0 || commandIndex == 0) {
                input.clear();
                return;
            };
            input.clear();
            input = commandHistory[commandIndex-1];
        }
        if (key == GLFW_KEY_C && (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                          glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
            abortRequested = true;
        }
    }
}

void Console::handleChar(unsigned int codepoint) {
    if (!visible) return;
    if (codepoint >= 32 && codepoint < 127)
        input.push_back((char)codepoint);
}

void Console::appendText(std::vector<float>& pts, int x, int y, const std::string& text) {
    for (size_t i = 0; i < text.size(); i++) {
        unsigned char c = text[i];
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (font8x8_basic[c][row] & (1 << col)) {
                    pts.push_back((float)(x + col + i*8));
                    pts.push_back((float)(y + row));
                }
            }
        }
    }
}

void Console::command_start() {
    grid->pause = false;
    if (cfg->vsync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }
}

void Console::command_stop() {
    grid->pause = true;
    glfwSwapInterval(1);
}

void Console::command_regen() {
    grid->initSeed();
    grid->initRandomGrid();
}

void Console::command_step(int n_step, float delay) {
    grid->pause = true;

    double lastTime = glfwGetTime();
    double remain_time = 0.0;

    if (!cfg->vsync) glfwSwapInterval(0);
    for (int i = 0; i < n_step;) {
        if (abortRequested) {
            log(std::format("Aborted. {} steps done.", i));
            break;
        }
        
        remain_time = delay - (glfwGetTime() - lastTime);
        if (remain_time < 0) {
            grid->step();
            ++i;
            lastTime = glfwGetTime();
        }
        
        renderer->render();
        draw(win->get());
        glfwSwapBuffers(win->get());
        glfwPollEvents();
    }
    if (!abortRequested) log(std::format("{} steps done.", n_step));

    glfwSwapInterval(1);

    abortRequested = false;
}

void Console::setWindowSize(int w, int h) {
    cfg->width = w;
    cfg->height = h;
    if (cfg->window) glfwSetWindowSize(cfg->window, w, h);
}

void Console::setGridSize(int x, int y) {
    cfg->gridx = x;
    cfg->gridy = y;
    grid->initSize();
    grid->initMask();
    grid->initRandomGrid();
    renderer->reset();
    renderer->render();
}

void Console::setRuleset(std::string rulestr) {
    cfg->rulestr = rulestr;
    auto [ok, msg] = cfg->parseRuleset(cfg->rulestr);
    if (!ok) {
        log(msg);
        auto [ok, msg] = cfg->parseRuleset("B3S23");
    }
    log(msg);
    grid->initRuleset();
}

void Console::setSeed(bool isRandom, int seed) {
    if (isRandom) {
        cfg->randomSeed = true;
    } else {
        cfg->randomSeed = false;
        cfg->seed = seed;
        grid->initSeed();
    }
    grid->initRandomGrid();
}

void Console::setDistrib(std::string distType, float density) {
    cfg->distType = distType;
    cfg->density = density;
    auto [ok, msg] = cfg->parseDistType(cfg->distType);
    if (!ok) {
        log(msg);
        auto [ok, msg] = cfg->parseDistType("uniform");
    }
    log(msg);
    grid->initRandomGrid();
}

void Console::getWindowSize() {
    log(std::format("Window size: {}x{}", cfg->width, cfg->height));
}

void Console::getGridSize() {
    log(std::format("Grid size: {}x{}", cfg->gridx, cfg->gridy));
}

void Console::cleanup() {

}