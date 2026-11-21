#include "audience/Audience.h"
#include "geometry/Primitives.h"

#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cstddef>
#include <vector>

namespace cgf {

MeshData createSpectatorMesh()
{
    MeshData spectator;
    spectator.name = "Low-poly spectator";
    auto box = [&](glm::vec3 size, glm::vec3 position, glm::vec3 color,
                   float angle = 0.0f) {
        MeshData part = makeBox(size, color);
        const glm::mat4 rotation = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 0, 1));
        const auto offset = static_cast<std::uint32_t>(spectator.vertices.size());
        for (Vertex vertex : part.vertices) {
            vertex.position = position + glm::vec3(rotation * glm::vec4(vertex.position, 1));
            vertex.normal = glm::mat3(rotation) * vertex.normal;
            spectator.vertices.push_back(vertex);
        }
        for (auto index : part.indices) spectator.indices.push_back(offset + index);
    };
    const glm::vec3 skin(0.85f, 0.61f, 0.43f);
    const glm::vec3 trousers(0.16f, 0.19f, 0.28f);
    const glm::vec3 shoes(0.08f, 0.07f, 0.10f);
    box({0.43f, 0.57f, 0.26f}, {0, 0.99f, 0}, {1, 1, 1});
    box({0.31f, 0.34f, 0.30f}, {0, 1.46f, 0}, skin);
    box({0.33f, 0.10f, 0.32f}, {0, 1.65f, 0.01f}, {0.19f, 0.12f, 0.09f});
    // Small eyes on the -Z face make the audience's orientation explicit.
    for (float side : {-1.0f, 1.0f}) {
        box({0.035f, 0.035f, 0.02f}, {side * 0.075f, 1.49f, -0.157f}, shoes);
        box({0.15f, 0.60f, 0.18f}, {side * 0.12f, 0.40f, 0}, trousers);
        box({0.18f, 0.10f, 0.30f}, {side * 0.12f, 0.05f, -0.04f}, shoes);
        // Raised arms in a cheering pose, joined to the torso.
        box({0.13f, 0.55f, 0.16f}, {side * 0.32f, 1.38f, 0},
            {1, 1, 1}, side * -0.35f);
        box({0.14f, 0.15f, 0.17f}, {side * 0.42f, 1.70f, 0}, skin);
    }
    return spectator;
}

namespace {
struct Instance {
    glm::vec4 positionScale;
    glm::vec4 tintPhase;
};
}

Audience::~Audience() { release(); }

void Audience::initialize()
{
    release();
    const MeshData mesh = createSpectatorMesh();
    indexCount_ = static_cast<GLsizei>(mesh.indices.size());
    const std::array<glm::vec3, 6> colors = {{
        {0.95f, 0.30f, 0.33f}, {0.25f, 0.65f, 0.95f}, {0.95f, 0.72f, 0.22f},
        {0.40f, 0.80f, 0.55f}, {0.73f, 0.42f, 0.90f}, {0.90f, 0.58f, 0.35f}
    }};
    std::vector<Instance> instances;
    // Fill row-by-row: all density levels keep the front rows occupied.
    for (int row = 0; row < 12; ++row) {
        for (int column = 0; column < 14; ++column) {
            const int id = row * 14 + column;
            const float jitter = static_cast<float>((id * 37) % 11 - 5) * 0.025f;
            const float scale = 0.85f + static_cast<float>((id * 17) % 8) * 0.035f;
            const glm::vec3 tint = colors[(id * 7 + row) % colors.size()];
            float x = (column - 6.5f) * 0.95f + jitter;
            float z = 8.5f + row * 1.0f + jitter;

            if (x > 3.5f && x < 6.5f && z < 12.0f)
            {
                z += 4.0f;
            }

            instances.push_back({
                {x, 0.0f, z, scale},
                {tint, static_cast<float>(id) * 2.399963f}
            });
        }
    }
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);
    glGenBuffers(1, &instanceBuffer_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(std::uint32_t), mesh.indices.data(), GL_STATIC_DRAW);
    const std::size_t offsets[] = {offsetof(Vertex, position), offsetof(Vertex, normal),
                                   offsetof(Vertex, uv), offsetof(Vertex, color)};
    for (GLuint attribute = 0; attribute < 4; ++attribute) {
        glEnableVertexAttribArray(attribute);
        glVertexAttribPointer(attribute, attribute == 2 ? 2 : 3, GL_FLOAT, GL_FALSE,
                              sizeof(Vertex), reinterpret_cast<void*>(offsets[attribute]));
    }
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer_);
    glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(Instance), instances.data(), GL_STATIC_DRAW);
    for (GLuint attribute = 4; attribute < 6; ++attribute) {
        glEnableVertexAttribArray(attribute);
        const auto offset = attribute == 4 ? offsetof(Instance, positionScale) : offsetof(Instance, tintPhase);
        glVertexAttribPointer(attribute, 4, GL_FLOAT, GL_FALSE, sizeof(Instance), reinterpret_cast<void*>(offset));
        glVertexAttribDivisor(attribute, 1);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Audience::draw(GLuint program, float time, bool energetic) const
{
    glUniform1i(glGetUniformLocation(program, "uAudience"), GL_TRUE);
    glUniform1f(glGetUniformLocation(program, "uAudienceTime"), time);
    glUniform1f(glGetUniformLocation(program, "uAudienceEnergy"), energetic ? 1.0f : 0.30f);
    glUniform3f(glGetUniformLocation(program, "uEmissionColor"), 0, 0, 0);
    glBindVertexArray(vao_);
    glDrawElementsInstanced(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr, visibleCount_);
    glBindVertexArray(0);
    glUniform1i(glGetUniformLocation(program, "uAudience"), GL_FALSE);
}

void Audience::cycleDensity()
{
    visibleCount_ = visibleCount_ == 168 ? 56 : (visibleCount_ == 56 ? 112 : 168);
}

void Audience::release()
{
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vertexBuffer_) glDeleteBuffers(1, &vertexBuffer_);
    if (indexBuffer_) glDeleteBuffers(1, &indexBuffer_);
    if (instanceBuffer_) glDeleteBuffers(1, &instanceBuffer_);
    vao_ = vertexBuffer_ = indexBuffer_ = instanceBuffer_ = 0;
}

} // namespace cgf
