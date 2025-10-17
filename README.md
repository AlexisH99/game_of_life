# Game of Life — C++ / OpenGL (Texture Renderer)

A real-time implementation of Conway’s Game of Life written in modern C++, using GLFW and GLAD.  
The simulation is computed on the CPU, while visualization is handled on the GPU through an OpenGL texture.

## Overview

The program displays a grid of living and dead cells.  
At each frame, the CPU updates the grid according to the Game of Life rules, uploads it as a texture, and the GPU renders it to the screen.

In real speed:

![gol_real_speed](https://i.imgur.com/E2rxiD0.gif)

## Bindings

| Key           | Action             |
| ------------- | ------------------ |
| F1            | open/close console |
| Space         | pause/unpause      |
| Arrow right   | do one step        |

## Commands (**WIP**)

By hitting F1, a console opens and a few commands are available:

| Command       | Args               | Action               |
| ------------- | ------------------ | -------------------- |
| start         | none               | unpause simulation   |
| stop          | none               | pause simulation     |
| reset         | none               | reset simulation     |
| step          | none               | do one step          |
| step          | \<n_steps\>        | do n_steps step      |
| resize        | \<w\> \<h\>        | resize window to w*h size (. as arg to keep previous value) |
| get           | \<globalProperty\> | print global property |

Global properties: `windowSize`, `gridSize`

Next things to implement : 

- More global properties
- Command to resize grid : reinitGrid and reinitRenderer to be done
- `step n_step` command in C++ side to get rendering between each step.
- `step n_step` command variant to get slowed loop (time per step as second argument)
- `help` command
- `set` or `toogle` commands for specific parameters
- Scrollable console (up to 1000 lines)
- Command history (up to 100 commands)
- Tab and autocompletion like in bash terminal

## Rules

The number of neighbors is computed according to the Moore neighborhood :

![moore_neighborhood](https://upload.wikimedia.org/wikipedia/commons/thumb/4/4d/Moore_neighborhood_with_cardinal_directions.svg/375px-Moore_neighborhood_with_cardinal_directions.svg.png)  

| Current State | Live Neighbors | Next State |
| ------------- | -------------- | ---------- |
| Alive         | <2 or >3       | Dead       |
| Alive         | 2 or 3         | Alive      |
| Dead          | 3              | Alive      |

## Concept

- The grid is stored in a vector, each row is represented by one to several words of `uint64_t`.
- Each cell is stored as a single bit in a `uint64_t` word.  
- Vectors `current` and `next`, store consecutive generations, while `mask` stores a mask of active cells.
- `step()` iterates overs `current` to compute `next` using efficient bitwise operations, solving 64 per 64 cells.  
- A single texture (`GL_RG32UI`) is updated each frame via `glTexSubImage2D`, by casting `current` as a vector of uint32_t.  
- Rendering uses one fullscreen quad drawn with a fragment shader that unpacks and samples the texture using paddings and bitwise operations.

This method avoids heavy instancing, providing excellent performance even for large grids. All computations on CPU are currently monothread.

## Project Structure

````
game_of_life/
├── src/
│ ├── main.cpp # Entry point
│ ├── app.cpp # Application class implementation
│ ├── config.cpp # Config class implementation
│ ├── console.cpp # Console class and LuaEngine class implementation
│ ├── gl_wrappers.cpp # OpenGL objects wrappers classes implementation
│ ├── grid.cpp # Grid class implementation
│ ├── renderer.cpp # Renderer class implementation
│ ├── shader.cpp # Shader class implementation
│ └── window.cpp # Window class implementation
├── include/
│ ├── app.hpp # Application class declaration
│ ├── config.hpp # Config class declaration
│ ├── console.hpp # Console class and LuaEngine class declaration
│ ├── font8x8_basic.hpp # Font for console as header-only file
│ ├── gl_wrappers.hpp # OpenGL objects wrappers classes declaration
│ ├── grid.hpp # Grid class declaration
│ ├── renderer.hpp # Renderer class declaration
│ ├── shader.hpp # Shader class declaration
│ ├── shaders_sources.hpp # GLSL shaders sources as header-only file
│ └── window.hpp # Window class declaration
├── resources/
│ ├── gol.rc.in # Application metadata
│ └── gol.ico # Icon .ico format
├── external/
│ └── glad/ # GLAD 2.0.8 vendored
└── CMakeLists.txt # Build configuration
````

## Dependencies

- **GLFW** – window creation and input handling  
- **GLAD** – OpenGL function loader
- **Nlohmann JSON** - JSON file parser  
- **CMake** – build system  
- **C++20** - compliant compiler

## Requirements

- **OpenGL 3.3+ (Core Profile)**  
  Compatible with all modern GPUs and drivers.  
  The program requires a graphics card supporting integer textures (`GL_RG32UI`) and GLSL 330 shaders.

### Compatible hardware
- **CPU :**

Any 64 bit CPU (x86-64)

- **GPU :**

NVIDIA : GeForce 8 series and newer (Tesla architecture, 2006+)  
AMD/ATI : Radeon HD 4000 series and newer (2008+)  
Intel : HD Graphics 2000 series and newer (2011+) 

Some older GPUs (AMD HD 2000–3000, Intel HD first gen) may not fully support `GL_RG32UI`. 

## Building - x86-64 Windows only (for now)

### 1. Clone the repository

```
git clone https://github.com/AlexisH99/game_of_life.git
cd game_of_life
```

### 2. Build

```
cmake -S . -B build -G "<your_generator>" -DCMAKE_BUILD_TYPE=Release
```
Tested generators : "mingw32-make", "Ninja"

### 3. Compile

```
cmake --build build
```

### 4. Run

```
./build/game_of_life
```

## Performance

Performed on Ryzen 5 9600X + GTX 980 Ti
| Grid Size   | v1.0.0 (FPS avg)                        | v1.1.0 (FPS avg)
| ----------- | --------------------------------------- | --------------------------------------- |
| 500 × 500   | ~1900 FPS                               | (Coming soon)                           |
| 1000 × 1000 | ~500 FPS                                |                                         |
| 2000 × 2000 | ~120 FPS                                |                                         |

Performance is now limited by rendering for small grids < 1000x1000. Then large grids > 5000x5000 are CPU limited.
A i7-6700HQ GTX960M laptop runs a 2000x2000 grid at ~950 fps on this dev branch, to be tested and released on v1.1.0.

## Planned features

- Save/Load grid from .bin or/and .png
- Zoom/Dezoom on the grid (Wheel for zoom/dezoom, mouse to move)
- Grid editor
- Command line interface (Lua) -> **WIP**
- Generalized rules for Moore neighborhood

## References

- [Conway’s Game of Life – Wikipedia](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life)  
  General description of the cellular automaton rules and mathematical background.

- [LearnOpenGL](https://learnopengl.com/)  
  Comprehensive tutorial series on modern OpenGL (3.3+), including shader compilation and texture rendering.

- [GLFW Documentation](https://www.glfw.org/docs/latest/)  
  Official reference for window creation, input handling, and context management.

- [GLAD OpenGL Loader Generator](https://glad.dav1d.de/)  
  Tool used to generate function loaders for OpenGL 3.3 Core Profile.

- [OpenGL 3.3 Core Specification](https://registry.khronos.org/OpenGL/specs/gl/glspec33.core.pdf)  
  Official Khronos specification describing all API functions and constants used in this project.
  
- [Nlohmann JSON documentation](https://json.nlohmann.me/)  
  Used JSON parser for user configuration. 

- [CMake Documentation](https://cmake.org/documentation/)  
  Official CMake reference for build configuration and compiler settings.

## License

This project is licensed under the MIT License.  
See the [LICENSE](./LICENSE) file for more details.
