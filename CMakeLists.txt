cmake_minimum_required(VERSION 3.20)
project(game_of_life LANGUAGES C CXX)

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

execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(VERSION_TAG ${GIT_DESCRIBE})
configure_file(${CMAKE_SOURCE_DIR}/resources/gol.rc.in
               ${CMAKE_BINARY_DIR}/gol.rc @ONLY)

set(APP_RC ${CMAKE_BINARY_DIR}/gol.rc)
set(APP_RES ${CMAKE_BINARY_DIR}/gol.res)

add_custom_command(
  OUTPUT ${APP_RES}
  COMMAND ${CMAKE_RC_COMPILER} ${APP_RC} -O coff -o ${APP_RES}
  DEPENDS ${APP_RC}
)

set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ -lpthread")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

add_executable(game_of_life 
    src/main.cpp
    src/app.cpp
    src/config.cpp
    src/grid.cpp
    src/window.cpp
    src/gl_wrappers.cpp
    src/shader.cpp
    src/renderer.cpp
    src/console.cpp
    ${APP_RES}
)

target_include_directories(game_of_life PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(game_of_life PRIVATE glfw glad nlohmann_json::nlohmann_json)
target_compile_options(game_of_life PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(game_of_life PRIVATE ${APP_RES})
