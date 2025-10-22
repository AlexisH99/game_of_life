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
    vboLogs = std::make_unique<GLBuffer>(GL_ARRAY_BUFFER);
    vboInput = std::make_unique<GLBuffer>(GL_ARRAY_BUFFER);
    vboSuggest = std::make_unique<GLBuffer>(GL_ARRAY_BUFFER);
    shaders = std::make_unique<GLProgram>(consoleVert, consoleFrag);

    log("Available commands:");
    log("  start / stop / regen");
    log("  step <n_steps> <delay>");
    log("  get <globalProperty>");
    log("  set <globalProperty> [values]");
    log("Available globalProperties:");
    log("  windowSize | gridSize | ruleSet | seed | dist");

    root.add("help", [&](const auto&) {
        log("Available commands:");
        log("  start / stop / regen");
        log("  step <n_steps> <delay>");
        log("  get <globalProperty>");
        log("  set <globalProperty> [values]");
        log("Available globalProperties:");
        log("  windowSize | gridSize | ruleSet | seed | dist");
    });
    
    root.add("start", [&](const auto& args){
        if (args.size() > 1) log("ignored arguments after 'start'");
        command_start();
    });
    root.add("stop",  [&](const auto& args){
        if (args.size() > 1) log("ignored arguments after 'stop'");
        command_stop();
    });
    root.add("regen", [&](const auto& args){
        if (args.size() > 1) log("ignored arguments after 'regen'");
        command_regen();
    });

    root.add("step", [&](const auto& args){
        if (args.size() == 1) command_step();
        else if (args.size() == 2) {
            auto n = from_string<int>(args[1]);
            if (!n) log("Usage: step <int>");
            else command_step(*n);
        } else if (args.size() == 3) {
            auto n = from_string<int>(args[1]);
            auto d = from_string<float>(args[2]);
            if (!n || !d) log("Usage: step <int> <float>");
            else command_step(*n, *d);
        } else log("Usage: step <int> [float]");
    });

    auto& get = root.add("get");
    get.add("windowSize", [&](auto&){ getWindowSize(); });
    get.add("gridSize",   [&](auto&){ getGridSize(); });

    auto& set = root.add("set");
    set.add("windowSize", [&](auto& args){
        if (args.size() != 4) {
            log("Usage: set windowSize <int|.> <int|.>");
            return;
        }
        auto w_str = args[2], h_str = args[3];

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
    });
    
    set.add("gridSize",   [&](auto& args){
        if (args.size() != 4) {
            log("Usage: set gridSize <int> <int>");
            return;
        }

        auto gx = from_string<int>(args[2]);
        auto gy = from_string<int>(args[3]);

        if (!gx || !gy) {
            log("Error: invalid arguments");
            log("Usage: set gridSize <int> <int>");
            return;
        }

        setGridSize(*gx, *gy);
        return;
    });

    set.add("ruleSet", [&](auto& args){
        if (args.size() != 2) {
            log("Usage: set ruleSet <str>");
            return;
        }
        setRuleset(args[1]);
        return;
    });

    set.add("seed", [&](auto& args){
        if (args.size() != 3) {
            log("Usage: set seed <str>/<int>");
            return;
        }
        auto seed = from_string<int>(args[2]);

        if (!seed) {
            if (args[2] == "rnd") {
                setSeed(true, 0);
                log("Random seed");
            } else {
                log("Error: invalid argument '" + args[2] + "'");
                log("Usage: set seed <str>/<int>");
            }
        } else {
            setSeed(false, *seed);
            log("Set seed to: " + std::to_string(grid->gridSeed));
        }
        return;
    });

    set.add("dist", [&](auto& args){
        if (args.size() != 3 && args.size() != 4) {
            log("Usage: set ruleSet <str> <float>");
            return;
        }
        auto distType = args[2];
        auto [ok, msg] = cfg->parseDistType(distType);
        if (args.size() == 3) {
            setDistrib(distType);
        }
        if (args.size() == 4) {
            auto density = from_string<float>(args[3]);
            setDistrib(distType, *density);
        }
        return;
    });    
}

template<typename T>
std::optional<T> Console::from_string(const std::string& s) {
    static_assert(std::is_arithmetic_v<T>, "T doit être un type numérique");

    T value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

    if (ec == std::errc()) return value;
    return std::nullopt;
}

const Console::CommandNode* Console::findNode(const CommandNode& root, const std::vector<std::string>& tokens) {
    const CommandNode* node = &root;
    for (const auto& t : tokens) {
        auto it = node->children.find(t);
        if (it == node->children.end()) {
            // On s'arrête dès que le prochain token n'existe plus :
            // les tokens restants sont des arguments
            break;
        }
        node = &it->second;
    }
    return node;
}

void Console::executeCommand(const CommandNode& root, const std::vector<std::string>& tokens) {
    const Console::CommandNode* node = findNode(root, tokens);
    if (!node) {
        log("Unknown command: " + tokens[0]);
        return;
    }
    if (node->action) node->action(tokens);
    else log("Incomplete command: missing subcommand");
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
    
    executeCommand(root, tokens);
}

std::vector<std::string> Console::suggest(const CommandNode& root, const std::vector<std::string>& tokens, bool endsWithSpace) {
    const CommandNode* node = &root;

    // Si on a des tokens, on parcourt l'arbre jusqu'au dernier token connu
    size_t limit = tokens.size();
    if (!endsWithSpace && !tokens.empty())
        limit = tokens.size() - 1;

    for (size_t i = 0; i < limit; ++i) {
        auto it = node->children.find(tokens[i]);
        if (it == node->children.end())
            return {};
        node = &it->second;
    }

    std::vector<std::string> matches;
    if (endsWithSpace) {
        // Liste tous les sous-commandes possibles du dernier nœud
        for (const auto& [key, child] : node->children)
            matches.push_back(key);
    } else if (!tokens.empty()) {
        const std::string& prefix = tokens.back();
        for (const auto& [key, child] : node->children) {
            if (key.starts_with(prefix))
                matches.push_back(key);
        }
    } else {
        // Aucun token (entrée vide)
        for (const auto& [key, child] : node->children)
            matches.push_back(key);
    }

    return matches;
}


void Console::draw(GLFWwindow* window) {
    if (!visible) return;
    pts.clear();
    ptsInput.clear();
    ptsSuggest.clear();
    shaders->use();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    cWidth = (float)fbWidth;
    cHeight = (float)(0.3 * fbHeight);

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

    int lineHeight = 10;
    maxVisibleLines = (int)(cHeight / lineHeight) - 3;
    if (maxVisibleLines < 1) maxVisibleLines = 1;

    int y = 10;
    int end = std::min((int)lines.size(), lineOffset + maxVisibleLines);
    for (int i = lineOffset; i < end; i++) {
        appendText(pts, 10, y, lines[i]);
        y += lineHeight;
    }

    auto tokens = std::vector<std::string>();
    {
        std::istringstream iss(input);
        std::string t;
        while (iss >> t) tokens.push_back(t);
    }

    suggestionText = "";
    endsWithSpace = !input.empty() && std::isspace(input.back());
    
    if (endsWithSpace) {
        matches = suggest(root, tokens, true);
        prefix = "";
    } else if (!tokens.empty()) {
        matches = suggest(root, tokens, false);
        prefix = tokens.back();
    } else {
        matches = suggest(root, tokens, false);
        prefix = "";
    }

    // Enregistre les suggestions courantes pour le cyclage
    currentSuggestions = matches;
    currentPrefix = prefix;

    // Reset index si besoin
    if (currentSuggestionIndex >= (int)currentSuggestions.size() || currentSuggestionIndex == -1)
        currentSuggestionIndex = 0;

    // Génère le texte affiché
    if (!currentSuggestions.empty()) {
        const std::string& full = currentSuggestions[currentSuggestionIndex];
        if (full.size() > prefix.size()) {
            suggestionText = full.substr(prefix.size());
        } else if (endsWithSpace) {
            suggestionText = full; // suggérer le mot entier
        }
    }

    std::string prompt = "> " + input;
    appendText(ptsInput, 10, cHeight - lineHeight, prompt);

    int inputPixelWidth = static_cast<int>(prompt.size() * 8);
    if (!suggestionText.empty()) {
        appendText(ptsSuggest, 10 + inputPixelWidth, cHeight - lineHeight, suggestionText);
    }

    if (!currentSuggestions.empty()) {
        std::string all = "[ ";
        for (int i = 0; i < (int)currentSuggestions.size(); ++i) {
            if (i == currentSuggestionIndex)
                all += ">"+currentSuggestions[i]+"< ";
            else
                all += currentSuggestions[i]+" ";
        }
        all += "]";
        appendText(ptsSuggest, 10, cHeight - 2*lineHeight, all);
    }

    vboLogs->bind();
    vboLogs->set_data(pts.size() * sizeof(float), pts.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glUniform4f(glGetUniformLocation(shaders->get(), "uColor"), 0.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_POINTS, 0, pts.size() / 2);

    // input
    vboInput->bind();
    vboInput->set_data(ptsInput.size() * sizeof(float), ptsInput.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glUniform4f(glGetUniformLocation(shaders->get(), "uColor"), 0.0f, 1.0f, 0.0f, 1.0f);
    glDrawArrays(GL_POINTS, 0, ptsInput.size() / 2);

    // suggestion
    vboSuggest->bind();
    vboSuggest->set_data(ptsSuggest.size() * sizeof(float), ptsSuggest.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);
    glUniform4f(glGetUniformLocation(shaders->get(), "uColor"), 0.9f, 0.15f, 0.15f, 0.8f);
    glDrawArrays(GL_POINTS, 0, ptsSuggest.size() / 2);

    glDisable(GL_BLEND);
}

void Console::handleInput([[maybe_unused]]GLFWwindow* win, int key, int action) {
    if (!visible) return;
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        
        if (key == GLFW_KEY_ENTER) {
            execute(input);
            input.clear();
            lineOffset = std::max(0, (int)lines.size() - maxVisibleLines);
        }

        else if (key == GLFW_KEY_BACKSPACE && !input.empty()) {
            input.pop_back();
            currentSuggestions.clear();
            currentSuggestionIndex = -1;
        }

        else if (key == GLFW_KEY_UP) {
            commandIndex += 1;
            commandIndex = std::clamp(commandIndex, 0, (int)commandHistory.size());
            if (commandHistory.size() == 0) return;
            input.clear();
            input = commandHistory[commandIndex-1];
        }

        else if (key == GLFW_KEY_DOWN) {
            commandIndex -= 1;
            commandIndex = std::clamp(commandIndex, 0, (int)commandHistory.size());
            if (commandHistory.size() == 0 || commandIndex == 0) {
                input.clear();
                return;
            };
            input.clear();
            input = commandHistory[commandIndex-1];
        }

        else if (key == GLFW_KEY_TAB) {
            if (!currentSuggestions.empty()) {
                if ((glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)) {
                    currentSuggestionIndex = (currentSuggestionIndex - 1) % currentSuggestions.size();
                    std::string next = currentSuggestions[currentSuggestionIndex];
                    std::cout << "Suggestion: " << next << std::endl;
                } else {
                    currentSuggestionIndex = (currentSuggestionIndex + 1) % currentSuggestions.size();
                    std::string next = currentSuggestions[currentSuggestionIndex];
                    std::cout << "Suggestion: " << next << std::endl;
                }
            }
        } 

        else if (key == GLFW_KEY_RIGHT) {
            if (!currentSuggestions.empty() && currentSuggestionIndex >= 0) {
                const std::string& full = currentSuggestions[currentSuggestionIndex];
                if (full.size() > currentPrefix.size()) {
                    input += full.substr(currentPrefix.size());
                    currentSuggestions.clear();
                    currentSuggestionIndex = -1;
                }
            }
        }

        else if (key == GLFW_KEY_C && (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                          glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
            abortRequested = true;
        }
    }
}

void Console::handleChar(unsigned int codepoint) {
    if (!visible) return;
    if (codepoint >= 32 && codepoint < 127) {
        input.push_back((char)codepoint);
        currentSuggestions.clear();
        currentSuggestionIndex = -1;
    }
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
    if (w < 800) {
        cfg->width = 800;
        log("minimum window width is 800");
    } else {
        cfg->width = w;
    }
    if (h < 600) {
        cfg->width = 600;
        log("minimum window height is 600");
    } else {
        cfg->width = h;
    }
    if (cfg->window) {
        glfwSetWindowSize(cfg->window, w, h);
        renderer->initRender();
        renderer->render();
    }
}

void Console::setGridSize(int x, int y) {
    cfg->gridx = x;
    cfg->gridy = y;
    grid->initSize();
    grid->initMask();
    grid->initRandomGrid();
    renderer->initRender();
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