cmake_minimum_required(VERSION 3.20)
project(game_of_life LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(external/glad)

add_executable(game_of_life 
    src/main.cpp
    src/app.cpp
    src/grid.cpp
    src/config.cpp
)

target_include_directories(game_of_life PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(game_of_life PRIVATE glfw glad nlohmann_json::nlohmann_json)
target_compile_options(game_of_life PRIVATE -Wall -Wextra -Wpedantic)
