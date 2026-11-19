#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <vector>
#include <random>

class ConfettiEffect
{
public:
    bool initialize();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void update(float deltaTime);

    void render(
        const glm::mat4& view,
        const glm::mat4& projection);

    void cleanup();

private:
    struct Particle
    {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 color;

        float size = 0.1f;
        float rotation = 0.0f;
        float rotationSpeed = 0.0f;
    };

    void resetParticle(Particle& particle, bool initial);
    float randomFloat(float minValue, float maxValue);

    std::vector<Particle> particles;
    std::mt19937 generator{ std::random_device{}() };

    GLuint shaderProgram = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;

    bool initialized = false;
    bool enabled = false;
};