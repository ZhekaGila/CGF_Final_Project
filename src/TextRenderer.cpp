#include "TextRenderer.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace
{
    std::string loadTextFile(const std::string& path)
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            std::cerr << "Cannot open text shader: " << path << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        return buffer.str();
    }

    GLuint compileTextShader(GLenum type, const std::string& source)
    {
        GLuint shader = glCreateShader(type);

        const char* sourcePointer = source.c_str();
        glShaderSource(shader, 1, &sourcePointer, nullptr);
        glCompileShader(shader);

        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            char infoLog[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);

            std::cerr << "Text shader compilation failed:\n"
                      << infoLog << std::endl;

            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    GLuint createTextShaderProgram()
    {
        std::string shaderDirectory = "shaders";

#ifdef CGF_SHADER_DIR
        shaderDirectory = CGF_SHADER_DIR;
#endif

        std::string vertexSource =
            loadTextFile(shaderDirectory + "/text.vert");

        std::string fragmentSource =
            loadTextFile(shaderDirectory + "/text.frag");

        if (vertexSource.empty() || fragmentSource.empty())
            return 0;

        GLuint vertexShader =
            compileTextShader(GL_VERTEX_SHADER, vertexSource);

        GLuint fragmentShader =
            compileTextShader(GL_FRAGMENT_SHADER, fragmentSource);

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

            std::cerr << "Text shader linking failed:\n"
                      << infoLog << std::endl;

            glDeleteProgram(program);
            program = 0;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }
}

bool TextRenderer::initialize(
    const std::string& fontPath,
    unsigned int fontSize)
{
    cleanup();

    FT_Library freeType;

    if (FT_Init_FreeType(&freeType))
    {
        std::cerr << "Failed to initialize FreeType!" << std::endl;
        return false;
    }

    FT_Face face;

    if (FT_New_Face(freeType, fontPath.c_str(), 0, &face))
    {
        std::cerr << "Failed to load font: " << fontPath << std::endl;
        FT_Done_FreeType(freeType);
        return false;
    }

    if (FT_Set_Pixel_Sizes(face, 0, fontSize))
    {
        std::cerr << "Failed to set font size!" << std::endl;
        FT_Done_Face(face);
        FT_Done_FreeType(freeType);
        return false;
    }

    GLint oldUnpackAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load the first 128 ASCII characters.
    for (unsigned char c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cerr << "Failed to load character: "
                      << static_cast<int>(c) << std::endl;
            continue;
        }

        GLuint texture = 0;

        if (face->glyph->bitmap.width > 0 &&
            face->glyph->bitmap.rows > 0)
        {
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer);

            glTexParameteri(
                GL_TEXTURE_2D,
                GL_TEXTURE_WRAP_S,
                GL_CLAMP_TO_EDGE);

            glTexParameteri(
                GL_TEXTURE_2D,
                GL_TEXTURE_WRAP_T,
                GL_CLAMP_TO_EDGE);

            glTexParameteri(
                GL_TEXTURE_2D,
                GL_TEXTURE_MIN_FILTER,
                GL_LINEAR);

            glTexParameteri(
                GL_TEXTURE_2D,
                GL_TEXTURE_MAG_FILTER,
                GL_LINEAR);
        }

        Character character =
        {
            texture,
            glm::ivec2(
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows),
            glm::ivec2(
                face->glyph->bitmap_left,
                face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };

        characters.insert(
            std::make_pair(static_cast<char>(c), character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpackAlignment);

    FT_Done_Face(face);
    FT_Done_FreeType(freeType);

    shaderProgram = createTextShaderProgram();

    if (shaderProgram == 0)
    {
        cleanup();
        return false;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(float) * 6 * 4,
        nullptr,
        GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        0,
        4,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glUseProgram(shaderProgram);

    glUniform1i(
        glGetUniformLocation(shaderProgram, "text"),
        0);

    glUseProgram(0);

    initialized = true;

    std::cout << "TextRenderer initialized successfully!" << std::endl;

    return true;
}

void TextRenderer::renderText(
    const std::string& text,
    float x,
    float y,
    float scale,
    const glm::vec3& color,
    int screenWidth,
    int screenHeight)
{
    if (!initialized || screenWidth <= 0 || screenHeight <= 0)
        return;

    // Save OpenGL states before drawing the HUD.
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);

    GLint previousProgram = 0;
    GLint previousVAO = 0;
    GLint previousArrayBuffer = 0;
    GLint previousActiveTexture = 0;
    GLint previousTexture = 0;

    GLint oldBlendSrcRGB = 0;
    GLint oldBlendDstRGB = 0;
    GLint oldBlendSrcAlpha = 0;
    GLint oldBlendDstAlpha = 0;

    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArrayBuffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);

    glGetIntegerv(GL_BLEND_SRC_RGB, &oldBlendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &oldBlendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldBlendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &oldBlendDstAlpha);

    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(
        0.0f,
        static_cast<float>(screenWidth),
        0.0f,
        static_cast<float>(screenHeight));

    glUniformMatrix4fv(
        glGetUniformLocation(shaderProgram, "projection"),
        1,
        GL_FALSE,
        &projection[0][0]);

    glUniform3f(
        glGetUniformLocation(shaderProgram, "textColor"),
        color.r,
        color.g,
        color.b);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    for (char c : text)
    {
        auto iterator = characters.find(c);

        if (iterator == characters.end())
            continue;

        const Character& character = iterator->second;

        float xpos =
            x + character.bearing.x * scale;

        float ypos =
            y - (character.size.y - character.bearing.y) * scale;

        float width =
            character.size.x * scale;

        float height =
            character.size.y * scale;

        if (width > 0.0f && height > 0.0f)
        {
            float vertices[6][4] =
            {
                { xpos,         ypos + height, 0.0f, 0.0f },
                { xpos,         ypos,          0.0f, 1.0f },
                { xpos + width, ypos,          1.0f, 1.0f },

                { xpos,         ypos + height, 0.0f, 0.0f },
                { xpos + width, ypos,          1.0f, 1.0f },
                { xpos + width, ypos + height, 1.0f, 0.0f }
            };

            glBindTexture(GL_TEXTURE_2D, character.textureID);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);

            glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                sizeof(vertices),
                vertices);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        x += (character.advance >> 6) * scale;
    }

    // Restore OpenGL states.
    glBindVertexArray(previousVAO);
    glBindBuffer(GL_ARRAY_BUFFER, previousArrayBuffer);

    glBindTexture(GL_TEXTURE_2D, previousTexture);
    glActiveTexture(previousActiveTexture);

    glUseProgram(previousProgram);

    glBlendFuncSeparate(
        oldBlendSrcRGB,
        oldBlendDstRGB,
        oldBlendSrcAlpha,
        oldBlendDstAlpha);

    if (depthWasEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    if (blendWasEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

void TextRenderer::cleanup()
{
    for (auto& pair : characters)
    {
        if (pair.second.textureID != 0)
            glDeleteTextures(1, &pair.second.textureID);
    }

    characters.clear();

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

    initialized = false;
}

bool TextRenderer::isInitialized() const
{
    return initialized;
}