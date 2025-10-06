# Game of Life — C++ / OpenGL (Texture Renderer)

A real-time implementation of Conway’s Game of Life written in modern C++, using GLFW and GLAD.  
The simulation is computed on the CPU, while visualization is handled on the GPU through an OpenGL texture.

---

## Overview

The program displays a grid of living and dead cells.  
At each frame, the CPU updates the grid according to the Game of Life rules, uploads it as a texture, and the GPU renders it to the screen.

---

## Concept

- Each cell is stored as an 8-bit unsigned integer (`uint8_t`) with values `0` (dead) or `255` (alive).  
- Two buffers, `current` and `next`, store consecutive generations.  
- The update loop is fully CPU-based and cache-efficient.  
- A single texture (`GL_R8`) is updated each frame via `glTexSubImage2D`.  
- Rendering uses one fullscreen quad drawn with a fragment shader that samples the texture.

This method avoids heavy instancing or GPU synchronization, providing excellent performance even for large grids.

---

## Project Structure

````
game_of_life/
├── src/
│ ├── main.cpp # Entry point
│ ├── app.cpp # Implementation (setup, loop, rendering)
├── include/
│ ├── app.hpp # Application class declaration
│ └── config.hpp # Constants (window and grid sizes)
├── external/
│ └── glad/ # GLAD 2.0.8 vendored
├── shaders/
│ ├── vertex.glsl # Vertex shader
│ └── fragment.glsl # Fragment shader
└── CMakeLists.txt # Build configuration
````
---

## Dependencies

- **GLFW** – window creation and input handling  
- **GLAD** – OpenGL function loader  
- **CMake** – build system  
- **C++20** - compliant compiler

---

## Building

### 1. Clone the repository

```
git clone https://github.com/<your-username>/game_of_life.git
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

| Grid Size   | Average FPS (Ryzen 5 9600X + GTX 980 Ti) |
| ----------- | --------------------------------------- |
| 500 × 500   | ~1900 FPS                               |
| 1000 × 1000 | ~500 FPS                                |
| 2000 × 2000 | ~120 FPS                                |

Performance is limited mainly by CPU memory bandwidth, not GPU rendering.
The GPU performs only one draw call per frame.

## Possible improvements

| Goal               | Description                                                    |
| ------------------ | -------------------------------------------------------------- |
| Multi-threaded CPU | Use OpenMP or `std::execution` to parallelize updates          |
| SIMD + bitpacking  | Optimizations allowing very large grids on CPU                 |
| User controls      | Add pause, speed adjustment, and reset features                |
| Persistence        | Save/load grid states (RLE, JSON, PNG)                         |
| Colorization       | Visualize cell age or energy levels                            |

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

- [CMake Documentation](https://cmake.org/documentation/)  
  Official CMake reference for build configuration and compiler settings.

## License

This project is licensed under the MIT License.  
See the [LICENSE](./LICENSE) file for more details.
