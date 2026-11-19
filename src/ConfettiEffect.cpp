#include "ConfettiEffect.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstddef>
#include <cmath>
#include <algorithm>

namespace
{
    struct ConfettiVertex
    {
        glm::vec3 position;
        glm::vec4 color;
    };

    std::string loadConfettiShader(const std::string &path)
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            std::cerr << "Cannot open confetti shader: "
                      << path << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        return buffer.str();
    }

    GLuint compileConfettiShader(
        GLenum type,
        const std::string &source)
    {
        GLuint shader = glCreateShader(type);

        const char *sourcePointer = source.c_str();

        glShaderSource(shader, 1, &sourcePointer, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            char infoLog[1024];

            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);

            std::cerr << "Confetti shader compilation failed:\n"
                      << infoLog << std::endl;

            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    GLuint createConfettiShaderProgram()
    {
        std::string shaderDirectory = "shaders";

#ifdef CGF_SHADER_DIR
        shaderDirectory = CGF_SHADER_DIR;
#endif

        std::string vertexSource =
            loadConfettiShader(shaderDirectory + "/confetti.vert");

        std::string fragmentSource =
            loadConfettiShader(shaderDirectory + "/confetti.frag");

        if (vertexSource.empty() || fragmentSource.empty())
            return 0;

        GLuint vertexShader =
            compileConfettiShader(GL_VERTEX_SHADER, vertexSource);

        GLuint fragmentShader =
            compileConfettiShader(GL_FRAGMENT_SHADER, fragmentSource);

        if (vertexShader == 0 || fragmentShader == 0)
        {
            if (vertexShader != 0)
                glDeleteShader(vertexShader);

            if (fragmentShader != 0)
                glDeleteShader(fragmentShader);

            return 0;
        }

        GLuint program = glCreateProgram();

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (!success)
        {
            char infoLog[1024];

            glGetProgramInfoLog(program, 1024, nullptr, infoLog);

            std::cerr << "Confetti shader linking failed:\n"
                      << infoLog << std::endl;

            glDeleteProgram(program);
            program = 0;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }
}

float ConfettiEffect::randomFloat(
    float minValue,
    float maxValue)
{
    std::uniform_real_distribution<float> distribution(
        minValue, maxValue);

    return distribution(generator);
}

void ConfettiEffect::resetParticle(
    Particle &particle,
    bool initial)
{
    // Cover the stage and the entire audience area.
    particle.position = glm::vec3(
        randomFloat(-8.0f, 8.0f),
        initial ? randomFloat(5.0f, 13.0f)
                : randomFloat(9.0f, 13.0f),
        randomFloat(-5.0f, 18.0f));

    particle.velocity = glm::vec3(
        randomFloat(-0.8f, 0.8f),
        randomFloat(-5.0f, -2.5f),
        randomFloat(-0.35f, 0.35f));

    particle.size = randomFloat(0.07f, 0.16f);
    particle.rotation = randomFloat(0.0f, 6.28f);
    particle.rotationSpeed = randomFloat(-5.0f, 5.0f);

    const glm::vec4 colors[] =
        {
            glm::vec4(1.0f, 0.15f, 0.15f, 0.95f),
            glm::vec4(0.1f, 0.8f, 1.0f, 0.95f),
            glm::vec4(1.0f, 0.85f, 0.1f, 0.95f),
            glm::vec4(1.0f, 0.2f, 0.8f, 0.95f),
            glm::vec4(0.2f, 1.0f, 0.35f, 0.95f),
            glm::vec4(1.0f, 0.45f, 0.05f, 0.95f),
            glm::vec4(0.55f, 0.25f, 1.0f, 0.95f)};

    int colorIndex = static_cast<int>(randomFloat(0.0f, 7.0f));

    if (colorIndex > 6)
        colorIndex = 6;

    particle.color = colors[colorIndex];
}

bool ConfettiEffect::initialize()
{
    cleanup();

    shaderProgram = createConfettiShaderProgram();

    if (shaderProgram == 0)
        return false;

    particles.resize(300);

    for (Particle &particle : particles)
        resetParticle(particle, true);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            particles.size() * 6 * sizeof(ConfettiVertex)),
        nullptr,
        GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(ConfettiVertex),
        reinterpret_cast<void *>(offsetof(ConfettiVertex, position)));

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(ConfettiVertex),
        reinterpret_cast<void *>(offsetof(ConfettiVertex, color)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initialized = true;

    std::cout << "Confetti effect initialized!" << std::endl;

    return true;
}

void ConfettiEffect::setEnabled(bool value)
{
    enabled = value;

    if (enabled)
    {
        for (Particle &particle : particles)
            resetParticle(particle, true);
    }
}

bool ConfettiEffect::isEnabled() const
{
    return enabled;
}

void ConfettiEffect::update(float deltaTime)
{
    if (!initialized || !enabled)
        return;

    // Prevent large jumps if the application freezes briefly.
    const float dt = std::min(deltaTime, 0.05f);

    for (Particle &particle : particles)
    {
        particle.position += particle.velocity * dt;

        particle.rotation += particle.rotationSpeed * dt;

        // Slight horizontal movement for a natural falling effect.
        particle.velocity.x +=
            std::sin(particle.rotation) * 0.15f * dt;

        if (particle.position.y < -1.5f)
            resetParticle(particle, false);
    }
}

void ConfettiEffect::render(
    const glm::mat4 &view,
    const glm::mat4 &projection)
{
    if (!initialized || !enabled)
        return;

    std::vector<ConfettiVertex> vertices;
    vertices.reserve(particles.size() * 6);

    for (const Particle &particle : particles)
    {
        const float halfWidth = particle.size;
        const float halfHeight = particle.size * 0.45f;

        const float cosine = std::cos(particle.rotation);
        const float sine = std::sin(particle.rotation);

        const glm::vec2 corners[] =
            {
                {-halfWidth, -halfHeight},
                {halfWidth, -halfHeight},
                {halfWidth, halfHeight},
                {-halfWidth, halfHeight}};

        ConfettiVertex quad[4];

        for (int i = 0; i < 4; i++)
        {
            float rotatedX =
                corners[i].x * cosine - corners[i].y * sine;

            float rotatedY =
                corners[i].x * sine + corners[i].y * cosine;

            quad[i].position = particle.position +
                               glm::vec3(rotatedX, rotatedY, 0.0f);

            quad[i].color = particle.color;
        }

        vertices.push_back(quad[0]);
        vertices.push_back(quad[1]);
        vertices.push_back(quad[2]);

        vertices.push_back(quad[0]);
        vertices.push_back(quad[2]);
        vertices.push_back(quad[3]);
    }

    GLint previousProgram = 0;
    GLint previousVAO = 0;
    GLint previousArrayBuffer = 0;

    GLint oldBlendSrcRGB = 0;
    GLint oldBlendDstRGB = 0;
    GLint oldBlendSrcAlpha = 0;
    GLint oldBlendDstAlpha = 0;

    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean previousDepthMask = GL_TRUE;

    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);

    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);

    glGetIntegerv(GL_BLEND_SRC_RGB, &oldBlendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &oldBlendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &oldBlendDstAlpha);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Keep depth testing, but do not write particle depths.
    glDepthMask(GL_FALSE);

    glUseProgram(shaderProgram);

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "view"),
        1,
        GL_FALSE,
        glm::value_ptr(view));

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"),
        1,
        GL_FALSE,
        glm::value_ptr(projection));

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertices.size() * sizeof(ConfettiVertex)),
        vertices.data(),
        GL_DYNAMIC_DRAW);

    glDrawArrays(
        GL_TRIANGLES,
        0,
        static_cast<GLsizei>(vertices.size()));

    // Restore previous OpenGL state.
    glBindVertexArray(previousVAO);
    glBindBuffer(GL_ARRAY_BUFFER, previousArrayBuffer);

    glUseProgram(previousProgram);

    glBlendFuncSeparate(
        oldBlendSrcRGB,
        oldBlendDstRGB,
        oldBlendSrcAlpha,
        oldBlendDstAlpha);

    glDepthMask(previousDepthMask);

    if (blendWasEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

void ConfettiEffect::cleanup()
{
    if (VBO != 0)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (VAO != 0)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }

    if (shaderProgram != 0)
    {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    particles.clear();

    initialized = false;
    enabled = false;
}