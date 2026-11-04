#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace cgf
{

    // Data describing a single vertex.
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 color;
    };

    // Geometry stored on the CPU.
    struct MeshData
    {
        std::string name;

        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
    };

    // One individual object in the scene.
    struct ScenePart
    {
        std::string name;

        MeshData mesh;

        glm::mat4 transform{1.0f};
    };

}