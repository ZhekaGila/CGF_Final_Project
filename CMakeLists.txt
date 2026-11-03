cmake_minimum_required(VERSION 3.20)

project(ConcertStage LANGUAGES C CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required libraries
find_package(glfw3 3.5 REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# GLAD library
add_library(glad STATIC
    external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)

# Main application
add_executable(ConcertStage
    src/main.cpp
    src/Camera.cpp
)

target_include_directories(ConcertStage PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(ConcertStage PRIVATE
    glfw
    glm::glm
    glad
    OpenGL::GL
)