# Game of Life — C++ / OpenGL (Texture Renderer)

A real-time implementation of Conway’s Game of Life written in modern C++, using GLFW and GLAD.  
The simulation is computed on the CPU, while visualization is handled on the GPU through an OpenGL texture.

---

## Overview

The program displays a grid of living and dead cells.  
At each frame, the CPU updates the grid according to the Game of Life rules, uploads it as a texture, and the GPU renders it to the screen.

In real speed:

![gol_real_speed](https://i.imgur.com/E2rxiD0.gif)

---

## Concept

- The grid is stored in a vector, each row is represented by one to several words of `uint64_t`.
- Each cell is stored as a single bit in a `uint64_t` word.  
- Vectors `current` and `next`, store consecutive generations, while `mask` stores a mask of active cells.
- `step()` iterates overs `current` to compute `next` using efficient bitwise operations, solving 64 per 64 cells.  
- A single texture (`GL_RG32UI`) is updated each frame via `glTexSubImage2D`, by casting `current` as a vector of uint32_t.  
- Rendering uses one fullscreen quad drawn with a fragment shader that unpacks and samples the texture using paddings and bitwise operations.

This method avoids heavy instancing, providing excellent performance even for large grids. All computations on CPU are currently monothread.

---

## Project Structure

````
game_of_life/
├── src/
│ ├── main.cpp # Entry point
│ ├── app.cpp # Implementation (setup, loop, rendering)
│ ├── grid.cpp # Grid implementation (generation, solving)
│ ├── config.cpp # Config parsinf implementation
├── include/
│ ├── app.hpp # Application class declaration
│ ├── grid.hpp # Grid class declaration
│ └── config.hpp # Config class declarations
├── external/
│ └── glad/ # GLAD 2.0.8 vendored
└── CMakeLists.txt # Build configuration
````
---

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

---

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

## Rules

| Current State | Live Neighbors | Next State |
| ------------- | -------------- | ---------- |
| Alive         | <2 or >3       | Dead       |
| Alive         | 2 or 3         | Alive      |
| Dead          | 3              | Alive      |

## Performance

Performed on Ryzen 5 9600X + GTX 980 Ti
| Grid Size   | v1.0.0 (FPS avg)                        | v1.1.0 (FPS avg)
| ----------- | --------------------------------------- | --------------------------------------- |
| 500 × 500   | ~1900 FPS                               | (Coming soon)                           |
| 1000 × 1000 | ~500 FPS                                |                                         |
| 2000 × 2000 | ~120 FPS                                |                                         |

Performance is now limited by rendering for small grids < 1000x1000. Then large grids > 5000x5000 are CPU limited.
A i7-6700HQ GTX960M laptop runs a 2000x2000 grid at ~950 fps on this dev branch, to be tested and released on v1.1.0.

## Possible improvements

| Goal               | Description                                                    |
| ------------------ | -------------------------------------------------------------- |
| Persistence        | Save/load grid states (RLE, JSON, PNG)                         |

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
