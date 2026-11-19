#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <map>
#include <string>

class TextRenderer
{
public:
    bool initialize(const std::string& fontPath, unsigned int fontSize);
    void renderText(
        const std::string& text,
        float x,
        float y,
        float scale,
        const glm::vec3& color,
        int screenWidth,
        int screenHeight);

    void cleanup();
    bool isInitialized() const;

private:
    struct Character
    {
        GLuint textureID;
        glm::ivec2 size;
        glm::ivec2 bearing;
        unsigned int advance;
    };

    std::map<char, Character> characters;

    GLuint shaderProgram = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;

    bool initialized = false;
};