
cmake_minimum_required(VERSION 3.21)

project(ConcertStage LANGUAGES C CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required libraries
find_package(glfw3 3.5 REQUIRED)
find_package(glm CONFIG REQUIRED)

# Find OpenGL
if(APPLE)
    # Use Apple's native OpenGL.framework
    set(_SAVED_FIND_FRAMEWORK "${CMAKE_FIND_FRAMEWORK}")
    set(CMAKE_FIND_FRAMEWORK ONLY)

    find_package(OpenGL REQUIRED)

    if(_SAVED_FIND_FRAMEWORK STREQUAL "")
        unset(CMAKE_FIND_FRAMEWORK)
    else()
        set(CMAKE_FIND_FRAMEWORK "${_SAVED_FIND_FRAMEWORK}")
    endif()

    unset(_SAVED_FIND_FRAMEWORK)
else()
    find_package(OpenGL REQUIRED)
endif()

# GLAD library
add_library(glad STATIC
    external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)

# Procedural stage geometry library
add_library(stage_geometry STATIC
    src/geometry/Primitives.cpp
    src/geometry/StageGeometry.cpp
)

target_include_directories(stage_geometry PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(stage_geometry PUBLIC
    glm::glm
)

# Main application
add_executable(ConcertStage
    src/main.cpp
)

target_include_directories(ConcertStage PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(ConcertStage PRIVATE
    glfw
    glad
    OpenGL::GL
    stage_geometry
)
