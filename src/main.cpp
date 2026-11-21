#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "geometry/StageGeometry.h"
#include "geometry/Primitives.h"
#include "audience/Audience.h"
#include "Camera.h"
#include "audio/AudioManager.h"
#include "TextRenderer.h"
#include "ConfettiEffect.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <algorithm>


std::string loadShaderSource(const std::string &filePath)
{
#ifdef CGF_SHADER_DIR
    const std::string resolvedPath = std::string(CGF_SHADER_DIR) +
                                     filePath.substr(filePath.find_last_of('/'));
#else
    const std::string resolvedPath = filePath;
#endif
    std::ifstream file(resolvedPath);

    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: " << filePath << std::endl;

        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

GLuint createBrickTexture()
{
    constexpr int size = 512;
    constexpr int brickWidth = 128;
    constexpr int brickHeight = 64;
    constexpr int mortar = 5;

    std::vector<unsigned char> pixels(size * size * 3);

    for (int y = 0; y < size; ++y)
    {
        int row = y / brickHeight;
        int localY = y % brickHeight;
        int offset = (row % 2) * (brickWidth / 2);

        for (int x = 0; x < size; ++x)
        {
            int localX = (x + offset) % brickWidth;

            bool isMortar =
                localX < mortar ||
                localY < mortar;

            int noise = (x * 17 + y * 31 +
                         (x * y) % 97) % 23;

            int index = (y * size + x) * 3;

            if (isMortar)
            {
                pixels[index + 0] = 38;
                pixels[index + 1] = 35;
                pixels[index + 2] = 38;
            }
            else
            {
                pixels[index + 0] = static_cast<unsigned char>(95 + noise);
                pixels[index + 1] = static_cast<unsigned char>(40 + noise / 2);
                pixels[index + 2] = static_cast<unsigned char>(37 + noise / 3);
            }
        }
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB8,
        size, size, 0,
        GL_RGB, GL_UNSIGNED_BYTE,
        pixels.data()
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}


GLuint compileShader(GLenum type, const char *source)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &source, nullptr);

    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[1024];

        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);

        std::cerr << "Shader compilation failed:\n"
                  << infoLog << std::endl;

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}


GLuint createShaderProgram()
{

    std::string vertexSource = loadShaderSource("shaders/stage.vert");

    std::string fragmentSource = loadShaderSource("shaders/stage.frag");

    if (vertexSource.empty() || fragmentSource.empty())
    {
        return 0;
    }

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource.c_str());

    if (vertexShader == 0)
        return 0;

    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str());

    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
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

        std::cerr << "Shader linking failed:\n"
                  << infoLog << std::endl;

        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

GLuint createGlowShaderProgram()
{
    std::string vertexSource =
        loadShaderSource("shaders/glow.vert");

    std::string fragmentSource =
        loadShaderSource("shaders/glow.frag");

    if (vertexSource.empty() || fragmentSource.empty())
    {
        std::cerr << "Could not load glow shaders!" << std::endl;
        return 0;
    }

    GLuint vertexShader = compileShader(
        GL_VERTEX_SHADER,
        vertexSource.c_str());

    if (vertexShader == 0)
        return 0;

    GLuint fragmentShader = compileShader(
        GL_FRAGMENT_SHADER,
        fragmentSource.c_str());

    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success != GL_TRUE)
    {
        char infoLog[1024];
        glGetProgramInfoLog(
            program, 1024, nullptr, infoLog);

        std::cerr << "Glow shader linking failed: "
                  << infoLog << std::endl;

        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


GLuint createLedGlowShaderProgram()
{
    std::string vertexSource =
        loadShaderSource("shaders/led_glow.vert");

    std::string fragmentSource =
        loadShaderSource("shaders/led_glow.frag");

    if (vertexSource.empty() || fragmentSource.empty())
    {
        std::cerr << "Could not load glow shaders!" << std::endl;
        return 0;
    }

    GLuint vertexShader = compileShader(
        GL_VERTEX_SHADER,
        vertexSource.c_str());

    if (vertexShader == 0)
        return 0;

    GLuint fragmentShader = compileShader(
        GL_FRAGMENT_SHADER,
        fragmentSource.c_str());

    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success != GL_TRUE)
    {
        char infoLog[1024];
        glGetProgramInfoLog(
            program, 1024, nullptr, infoLog);

        std::cerr << "Glow shader linking failed: "
                  << infoLog << std::endl;

        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


struct GpuMesh
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    GLsizei indexCount = 0;

    glm::mat4 model = glm::mat4(1.0f);

    std::string name;
    glm::mat4 originalModel = glm::mat4(1.0f);
};


GpuMesh uploadMesh(const cgf::ScenePart &part)
{
    GpuMesh gpuMesh;

    gpuMesh.name = part.name;
    gpuMesh.originalModel = part.transform;

    gpuMesh.model = part.transform;

    gpuMesh.indexCount =
        static_cast<GLsizei>(part.mesh.indices.size());

    glGenVertexArrays(1, &gpuMesh.VAO);
    glGenBuffers(1, &gpuMesh.VBO);
    glGenBuffers(1, &gpuMesh.EBO);

    glBindVertexArray(gpuMesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            part.mesh.vertices.size() * sizeof(cgf::Vertex)),
        part.mesh.vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            part.mesh.indices.size() * sizeof(std::uint32_t)),
        part.mesh.indices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(cgf::Vertex),
        reinterpret_cast<void *>(offsetof(cgf::Vertex, position)));

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(cgf::Vertex),
        reinterpret_cast<void *>(offsetof(cgf::Vertex, normal)));

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(cgf::Vertex),
        reinterpret_cast<void *>(offsetof(cgf::Vertex, uv)));

    glEnableVertexAttribArray(2);

    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(cgf::Vertex),
        reinterpret_cast<void *>(offsetof(cgf::Vertex, color)));

    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    return gpuMesh;
}

void animateSpotlightMesh(
    GpuMesh &mesh,
    int spotlightNumber,
    float angle)
{
    std::string number = std::to_string(spotlightNumber);

    if (mesh.name != "Spotlight Fixture " + number &&
        mesh.name != "Spotlight Bulb " + number)
    {
        return;
    }

    float x = -5.0f + (spotlightNumber - 1) * 2.5f;

    glm::vec3 pivot(x, 5.5f, -2.5f);

    glm::mat4 parentTransform(1.0f);

    parentTransform = glm::translate(
        parentTransform, pivot);

    parentTransform = glm::rotate(
        parentTransform,
        angle,
        glm::vec3(0.0f, 0.0f, 1.0f));

    parentTransform = glm::translate(
        parentTransform, -pivot);

    mesh.model = parentTransform * mesh.originalModel;
}

void animateCentralSpotlight(
    GpuMesh &mesh,
    const glm::vec3 &direction)
{
    if (mesh.name != "Spotlight Fixture 3" &&
        mesh.name != "Spotlight Bulb 3")
    {
        return;
    }

    glm::vec3 pivot(0.0f, 5.5f, -2.5f);

    float angleZ = std::atan2(
        direction.x,
        -direction.y);

    float angleX = std::atan2(
        -direction.z,
        std::sqrt(
            direction.x * direction.x +
            direction.y * direction.y));

    glm::mat4 parentTransform(1.0f);

    parentTransform = glm::translate(
        parentTransform,
        pivot);

    parentTransform = glm::rotate(
        parentTransform,
        angleZ,
        glm::vec3(0.0f, 0.0f, 1.0f));

    parentTransform = glm::rotate(
        parentTransform,
        angleX,
        glm::vec3(1.0f, 0.0f, 0.0f));

    parentTransform = glm::translate(
        parentTransform,
        -pivot);

    mesh.model = parentTransform * mesh.originalModel;
}

void animatePerformer(GpuMesh &mesh, float time)
{
    glm::vec3 pivot;
    float phase;

    if (mesh.name.rfind("Drummer ", 0) == 0)
    {
        pivot = glm::vec3(-3.0f, 0.0f, -3.35f);
        phase = 0.0f;
    }
    else if (mesh.name.rfind("Singer ", 0) == 0)
    {
        pivot = glm::vec3(0.0f, 0.0f, 1.2f);
        phase = 1.2f;
    }
    else if (mesh.name.rfind("Guitarist ", 0) == 0)
    {
        pivot = glm::vec3(3.0f, 0.0f, 0.0f);
        phase = 2.4f;
    }
    else
    {
        return;
    }

    const float bodySway = std::sin(time * 1.8f + phase) * 0.055f;
    const bool upperBody = mesh.name.find("Torso") != std::string::npos ||
                           mesh.name.find("Head") != std::string::npos ||
                           mesh.name.find("Neck") != std::string::npos ||
                           mesh.name.find("Arm") != std::string::npos ||
                           mesh.name.find("Elbow") != std::string::npos ||
                           mesh.name.find("Hand") != std::string::npos;
    const float bounce = upperBody
                             ? std::max(0.0f, std::sin(time * 3.6f + phase)) * 0.035f
                             : 0.0f;

    glm::mat4 motion = glm::translate(glm::mat4(1.0f),
                                      glm::vec3(0.0f, bounce, 0.0f));
    motion = glm::translate(motion, pivot);
    motion = glm::rotate(motion, bodySway, glm::vec3(0.0f, 0.0f, 1.0f));
    motion = glm::translate(motion, -pivot);

    glm::vec3 armPivot(0.0f);
    float armSwing = 0.0f;
    const float beat = time * 7.0f;

    if (mesh.name.rfind("Drummer Left ", 0) == 0 &&
        (mesh.name.find("Upper Arm") != std::string::npos ||
         mesh.name.find("Forearm") != std::string::npos ||
         mesh.name.find("Elbow") != std::string::npos ||
         mesh.name.find("Hand") != std::string::npos ||
         mesh.name.find("Drumstick") != std::string::npos))
    {
        armPivot = glm::vec3(-3.34f, 1.78f, -3.22f);
        armSwing = std::sin(beat + phase) * 0.24f;
    }
    else if (mesh.name.rfind("Drummer Right ", 0) == 0 &&
             (mesh.name.find("Upper Arm") != std::string::npos ||
              mesh.name.find("Forearm") != std::string::npos ||
              mesh.name.find("Elbow") != std::string::npos ||
              mesh.name.find("Hand") != std::string::npos ||
              mesh.name.find("Drumstick") != std::string::npos))
    {
        armPivot = glm::vec3(-2.66f, 1.78f, -3.22f);
        armSwing = std::sin(beat + phase + 1.5f) * 0.24f;
    }
    else if (mesh.name.rfind("Singer Left ", 0) == 0 &&
             (mesh.name.find("Upper Arm") != std::string::npos ||
              mesh.name.find("Forearm") != std::string::npos ||
              mesh.name.find("Elbow") != std::string::npos ||
              mesh.name.find("Hand") != std::string::npos))
    {
        armPivot = glm::vec3(-0.27f, 1.78f, 1.36f);
        armSwing = std::sin(time * 2.2f + phase) * 0.22f;
    }
    else if (mesh.name.rfind("Guitarist Right ", 0) == 0 &&
             (mesh.name.find("Upper Arm") != std::string::npos ||
              mesh.name.find("Forearm") != std::string::npos ||
              mesh.name.find("Elbow") != std::string::npos ||
              mesh.name.find("Hand") != std::string::npos))
    {
        armPivot = glm::vec3(2.72f, 1.78f, 0.12f);
        armSwing = std::sin(beat + phase) * 0.18f;
    }

    if (armSwing != 0.0f)
    {
        glm::mat4 armMotion(1.0f);
        armMotion = glm::translate(armMotion, armPivot);
        armMotion = glm::rotate(armMotion, armSwing,
                                glm::vec3(0.0f, 0.0f, 1.0f));
        armMotion = glm::translate(armMotion, -armPivot);
        motion = armMotion * motion;
    }

    mesh.model = motion * mesh.originalModel;
}


void deleteGpuMeshes(std::vector<GpuMesh> &meshes)
{
    for (GpuMesh &mesh : meshes)
    {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
    }

    meshes.clear();
}


struct SpotLight
{
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
};

float smoothStep01(float value)
{
    value = std::max(0.0f, std::min(1.0f, value));
    return value * value * (3.0f - 2.0f * value);
}

void pointCameraAt(Camera &camera, const glm::vec3 &position,
                   const glm::vec3 &target)
{
    glm::vec3 front = glm::normalize(target - position);
    const float yaw = glm::degrees(std::atan2(front.z, front.x));
    const float pitch = glm::degrees(std::asin(glm::clamp(front.y, -1.0f, 1.0f)));
    camera.setPose(position, yaw, pitch);
}


GLuint createSoundWaveProgram()
{
    std::string vs = loadShaderSource("shaders/sound_wave.vert");
    std::string fs = loadShaderSource("shaders/sound_wave.frag");

    if (vs.empty() || fs.empty())
        return 0;

    GLuint vertex = compileShader(GL_VERTEX_SHADER, vs.c_str());
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fs.c_str());

    if (!vertex || !fragment)
    {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked)
    {
        char log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Sound wave shader error: " << log << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return program;
}


int main()
{

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_DEPTH_BITS, 24);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


    GLFWwindow *window = glfwCreateWindow(
        1280,
        720,
        "Concert Stage - 3D Scene",
        nullptr,
        nullptr);

    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;

        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);


    if (!gladLoadGLLoader(
            reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD!" << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    std::cout << "OpenGL initialized successfully!" << std::endl;
    TextRenderer textRenderer;
    ConfettiEffect confetti;

    if (!confetti.initialize())
    {
        std::cerr << "Confetti initialization failed!"
                  << std::endl;
    }

#ifdef _WIN32
    const std::string fontPath = "C:/Windows/Fonts/arial.ttf";
#elif defined(__APPLE__)
    const std::string fontPath =
        "/System/Library/Fonts/Supplemental/Arial.ttf";
#else
    const std::string fontPath =
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif

    if (!textRenderer.initialize(fontPath, 24))
    {
        std::cerr << "FPS HUD initialization failed. "
                  << "The 3D scene will continue without HUD."
                  << std::endl;
    }
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);


    GLuint soundWaveProgram = createSoundWaveProgram();

    constexpr int waveSegments = 256;
    std::vector<float> waveVertices;

    for (int i = 0; i < waveSegments; ++i)
    {
        float angle = 2.0f * 3.14159265f * i / waveSegments;

        waveVertices.push_back(std::cos(angle));
        waveVertices.push_back(std::sin(angle));
    }

    GLuint waveVAO = 0;
    GLuint waveVBO = 0;

    glGenVertexArrays(1, &waveVAO);
    glGenBuffers(1, &waveVBO);

    glBindVertexArray(waveVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waveVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        waveVertices.size() * sizeof(float),
        waveVertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        2 * sizeof(float), nullptr
    );

    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    float soundWaveTime = 0.0f;
    float soundWaveVisibility = 0.0f;


    GLuint shaderProgram = createShaderProgram();

    if (shaderProgram == 0)
    {
        std::cerr << "Could not create shader program!" << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    GLuint brickTexture = createBrickTexture();

    glUseProgram(shaderProgram);
    glUniform1i(
        glGetUniformLocation(shaderProgram, "uTexture"),
        0
    );

    GLuint glowShaderProgram = createGlowShaderProgram();

    if (glowShaderProgram == 0)
    {
        std::cerr << "Could not create glow shader program!" << std::endl;
        return -1;
    }

    GLuint ledGlowShaderProgram = createLedGlowShaderProgram();

    if (ledGlowShaderProgram == 0)
    {
        std::cerr << "Could not create LED glow shader!" << std::endl;
        return -1;
    }

    float glowVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,

        -1.0f, -1.0f,
        1.0f, 1.0f,
        -1.0f, 1.0f};

    GLuint glowVAO = 0;
    GLuint glowVBO = 0;

    glGenVertexArrays(1, &glowVAO);
    glGenBuffers(1, &glowVBO);

    glBindVertexArray(glowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, glowVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(glowVertices),
        glowVertices,
        GL_STATIC_DRAW);

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr);

    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "Glow billboard geometry created!"
              << std::endl;

    std::cout << "Glow shaders compiled successfully!" << std::endl;

    GLint modelLocation =
        glGetUniformLocation(shaderProgram, "uModel");

    GLint viewLocation =
        glGetUniformLocation(shaderProgram, "uView");

    GLint projectionLocation =
        glGetUniformLocation(shaderProgram, "uProjection");

    GLint cameraPosLocation = glGetUniformLocation(shaderProgram, "uCameraPos");


    std::vector<cgf::ScenePart> scene = cgf::createStageGeometry();

    const glm::vec3 stageOffset(0.0f, 1.5f, 0.0f);

    const glm::mat4 stageTransform =
        glm::translate(glm::mat4(1.0f), stageOffset);

    cgf::ScenePart audienceFloor;
    audienceFloor.name = "Audience floor";
    audienceFloor.mesh = cgf::makeBox({18.0f, 0.2f, 32.0f}, {0.10f, 0.11f, 0.15f});
    audienceFloor.transform = glm::translate(glm::mat4(1), {0.0f, -0.15f, 10.0f});
    scene.push_back(audienceFloor);

    std::cout << "Stage geometry created!" << std::endl;
    std::cout << "Number of scene objects: "
              << scene.size() << std::endl;


    std::vector<GpuMesh> gpuMeshes;

    gpuMeshes.reserve(scene.size());

    for (const cgf::ScenePart &part : scene)
    {
        if (part.mesh.vertices.empty() || part.mesh.indices.empty())
        {
            std::cerr << "Skipping empty mesh: "
                      << part.name << std::endl;

            continue;
        }

        gpuMeshes.push_back(uploadMesh(part));

        std::cout << "Uploaded: " << part.name << std::endl;
    }

    std::cout << "All meshes uploaded!" << std::endl;


    cgf::Audience audience;
    audience.initialize();

    AudioManager audio;

    float musicVolume = 0.70f;
    bool musicHasStarted = false;
    bool audioReady = false;

    const std::string musicPath =
        std::string(CGF_ASSET_DIR) + "/audio/concert_music.mp3";

    std::ifstream musicFile(musicPath, std::ios::binary);

    if (musicFile.good())
    {
        musicFile.close();

        audioReady = audio.initialize(musicPath, true);

        if (audioReady)
        {
            audio.setVolume(musicVolume);

            std::cout << "Audio system initialized!" << std::endl;
        }
    }
    else
    {
        std::cerr << "Music file not found: "
                  << musicPath << std::endl;
    }
    bool audiencePaused = false, audienceEnergetic = true, audienceVisible = true;
    bool previousP = false, previousJ = false, previousN = false, previousH = false;
    float audienceTime = 0.0f;
    double lastFrameTime = glfwGetTime(), fpsStart = lastFrameTime;
    int fpsFrames = 0;
    int currentFPS = 0;
    std::cout << "Camera: WASD move | Q/E down/up | Shift fast | Mouse look | Scroll zoom\n";
    std::cout << "Audience: P pause | J energy | N density | H hide | Esc exit\n";

    Camera camera(
        glm::vec3(0.0f, 6.0f, 24.0f),
        -90.0f,
        -10.0f);

    constexpr float pi = 3.14159265358979323846f;
    bool cinematicMode = false;
    float cinematicTime = 0.0f;
    float cinematicBlend = 0.0f;
    Camera cinematicStart = camera;
    Camera wowStart = camera;
    bool previousC = false, previousV = false;
    bool previousF1 = false, previousF2 = false;
    bool previousF3 = false, previousF4 = false;
    bool previous4 = false, previous5 = false;
    bool previous6 = false, previous7 = false;

    auto resetCamera = [&]()
    {
        camera = Camera(glm::vec3(0.0f, 6.0f, 24.0f),-90.0f,-10.0f);
        cinematicMode = false;
        cinematicTime = 0.0f;
        cinematicBlend = 0.0f;
    };

    auto switchToView = [&](const glm::vec3 &position, const glm::vec3 &target)
    {
        cinematicMode = false;
        cinematicBlend = 0.0f;
        pointCameraAt(camera, position, target);
    };

    glfwSetWindowUserPointer(window, &camera);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetCursorPosCallback(window,
                             [](GLFWwindow *win, double xpos, double ypos)
                             {
                                 static bool firstMouse = true;
                                 static double lastX = 0.0;
                                 static double lastY = 0.0;

                                 if (firstMouse)
                                 {
                                     lastX = xpos;
                                     lastY = ypos;
                                     firstMouse = false;
                                     return;
                                 }

                                 float xOffset = static_cast<float>(xpos - lastX);
                                 float yOffset = static_cast<float>(lastY - ypos);
                                 lastX = xpos;
                                 lastY = ypos;

                                 Camera *cam = static_cast<Camera *>(glfwGetWindowUserPointer(win));
                                 if (cam)
                                     cam->processMouseMovement(xOffset, yOffset);
                             });

    glfwSetScrollCallback(window,
                          [](GLFWwindow *win, double, double yOffset)
                          {
                              Camera *cam = static_cast<Camera *>(glfwGetWindowUserPointer(win));
                              if (cam)
                                  cam->processMouseScroll(static_cast<float>(yOffset));
                          });

    int lightingMode = 1; 
    float showSpeed = 1.0f;
    float showTime = 0.0f;
    float spotlightBrightness = 0.0f;

    float musicBPM = 120.0f;
    float beatTime = 0.0f;
    float beatPulse = 0.0f;

    float wowRemaining = 0.0f;
    float wowElapsed = 0.0f;
    constexpr float wowDuration = 5.0f;
    bool previous1 = false, previous2 = false, previous3 = false;

    bool previousPlus = false;
    bool previousMinus = false;
    bool previousReset = false;

    bool previousSpace = false;

    bool previousMusic = false;
    bool previousConfetti = false;
    bool previousVolumePlus = false;
    bool previousVolumeMinus = false;
    std::cout << "Lighting: 1 colorful | 2 warm | 3 cool | [ ] speed | R reset | Space WOW\n";

    std::cout << "Audio: M Play/Pause | - Volume Down | + Volume Up\n";
    std::cout << "Camera views: C reset | V cinematic on/off | F1/4 wide | F2/5 stage | F3/6 band | F4/7 crowd\n";

    SpotLight lights[5] =
        {
            {glm::vec3(-5.0f, 4.95f, -2.5f),
             glm::vec3(0.0f, -1.0f, 0.0f),
             glm::vec3(1.0f, 0.1f, 0.1f)},

            {glm::vec3(-2.5f, 4.95f, -2.5f),
             glm::vec3(0.0f, -1.0f, 0.0f),
             glm::vec3(0.1f, 0.3f, 1.0f)},

            {glm::vec3(2.5f, 4.95f, -2.5f),
             glm::vec3(0.0f, -1.0f, 0.0f),
             glm::vec3(0.1f, 1.0f, 0.2f)},

            {glm::vec3(5.0f, 4.95f, -2.5f),
             glm::vec3(0.0f, -1.0f, 0.0f),
             glm::vec3(0.8f, 0.1f, 1.0f)},

            {glm::vec3(0.0f, 4.95f, -2.5f),
             glm::vec3(0.0f, -1.0f, 0.0f),
             glm::vec3(1.0f, 0.88f, 0.65f)}};


    while (!glfwWindowShouldClose(window))
    {

        const double now = glfwGetTime();
        const float deltaTime = static_cast<float>(now - lastFrameTime);
        lastFrameTime = now;
        float targetBrightness = audio.isPlaying() ? 1.0f : 0.0f;

        spotlightBrightness +=
            (targetBrightness - spotlightBrightness) *
            std::min(1.0f, deltaTime * 2.0f);

        if (audio.isPlaying())
            soundWaveTime += deltaTime;

        float targetWaveVisibility = audio.isPlaying() ? 1.0f : 0.0f;

        soundWaveVisibility +=
            (targetWaveVisibility - soundWaveVisibility) *
            std::min(1.0f, deltaTime * 4.0f);

        confetti.update(deltaTime);

        const bool fast =
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

        if (!cinematicMode && wowRemaining <= 0.0f &&
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.processKeyboard(Camera::Movement::Forward, deltaTime, fast);
        if (!cinematicMode && wowRemaining <= 0.0f &&
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.processKeyboard(Camera::Movement::Backward, deltaTime, fast);
        if (!cinematicMode && wowRemaining <= 0.0f &&
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.processKeyboard(Camera::Movement::Left, deltaTime, fast);
        if (!cinematicMode && wowRemaining <= 0.0f &&
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.processKeyboard(Camera::Movement::Right, deltaTime, fast);
        if (!cinematicMode && wowRemaining <= 0.0f &&
            glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.processKeyboard(Camera::Movement::Down, deltaTime, fast);
        if (!cinematicMode && wowRemaining <= 0.0f &&
            glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.processKeyboard(Camera::Movement::Up, deltaTime, fast);

        auto pressed = [&](int key, bool &previous)
        {
            const bool down = glfwGetKey(window, key) == GLFW_PRESS;
            const bool edge = down && !previous;
            previous = down;
            return edge;
        };

        auto pressedState = [&](bool down, bool &previous)
        {
            const bool edge = down && !previous;
            previous = down;
            return edge;
        };


        if (pressed(GLFW_KEY_M, previousMusic))
        {
            if (audioReady)
            {
                if (audio.isPlaying())
                {
                    audio.pause();
                    std::cout << "Music paused" << std::endl;
                }
                else
                {
                    if (!musicHasStarted)
                    {
                        audio.play();
                        musicHasStarted = true;
                    }
                    else
                    {
                        audio.resume();
                    }

                    std::cout << "Music resumed" << std::endl;
                }
            }
            else
            {
                std::cout << "Audio is not initialized. Check music file."
                          << std::endl;
            }
        }

        if (pressed(GLFW_KEY_K, previousConfetti))
        {
            confetti.setEnabled(!confetti.isEnabled());

            std::cout << "Confetti: "
                      << (confetti.isEnabled() ? "ON" : "OFF")
                      << std::endl;
        }


        const bool shiftPressed =
            glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

        const bool plusDown =
            (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS && shiftPressed) ||
            glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS;

        const bool minusDown =
            glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS;

        if (pressedState(plusDown, previousVolumePlus))
        {
            musicVolume = std::min(1.0f, musicVolume + 0.05f);

            if (audioReady)
                audio.setVolume(musicVolume);

            std::cout << "Volume: "
                      << static_cast<int>(musicVolume * 100.0f)
                      << "%" << std::endl;
        }

        if (pressedState(minusDown, previousVolumeMinus))
        {
            musicVolume = std::max(0.0f, musicVolume - 0.05f);

            if (audioReady)
                audio.setVolume(musicVolume);

            std::cout << "Volume: "
                      << static_cast<int>(musicVolume * 100.0f)
                      << "%" << std::endl;
        }
        if (pressed(GLFW_KEY_P, previousP))
            audiencePaused = !audiencePaused;
        if (pressed(GLFW_KEY_J, previousJ))
            audienceEnergetic = !audienceEnergetic;
        if (pressed(GLFW_KEY_N, previousN))
            audience.cycleDensity();
        if (pressed(GLFW_KEY_H, previousH))
            audienceVisible = !audienceVisible;
        if (pressed(GLFW_KEY_1, previous1))
            lightingMode = 1;
        if (pressed(GLFW_KEY_2, previous2))
            lightingMode = 2;
        if (pressed(GLFW_KEY_3, previous3))
            lightingMode = 3;
        if (pressed(GLFW_KEY_RIGHT_BRACKET, previousPlus))
            showSpeed = std::min(3.0f, showSpeed + 0.25f);

        if (pressed(GLFW_KEY_LEFT_BRACKET, previousMinus))
            showSpeed = std::max(0.25f, showSpeed - 0.25f);
        if (pressed(GLFW_KEY_R, previousReset))
            showSpeed = 1.0f;
        if (pressed(GLFW_KEY_C, previousC))
        {
            wowRemaining = 0.0f;
            wowElapsed = 0.0f;
            resetCamera();
        }
        const bool view1Fn = pressed(GLFW_KEY_F1, previousF1);
        const bool view1Num = pressed(GLFW_KEY_4, previous4);
        if (view1Fn || view1Num)
        {
            wowRemaining = 0.0f;
            wowElapsed = 0.0f;
            switchToView({0.0f, 6.0f, 24.0f}, {0.0f, 2.0f, 0.0f});
        }
        const bool view2Fn = pressed(GLFW_KEY_F2, previousF2);
        const bool view2Num = pressed(GLFW_KEY_5, previous5);
        if (view2Fn || view2Num)
        {
            wowRemaining = 0.0f;
            wowElapsed = 0.0f;
            switchToView({0.0f, 7.0f, 21.0f}, {0.0f, 2.0f, -1.0f});
        }
        const bool view3Fn = pressed(GLFW_KEY_F3, previousF3);
        const bool view3Num = pressed(GLFW_KEY_6, previous6);
        if (view3Fn || view3Num)
        {
            wowRemaining = 0.0f;
            wowElapsed = 0.0f;
            switchToView({4.0f, 3.8f, 10.0f}, {0.0f, 1.8f, 0.0f});
        }
        const bool view4Fn = pressed(GLFW_KEY_F4, previousF4);
        const bool view4Num = pressed(GLFW_KEY_7, previous7);
        if (view4Fn || view4Num)
        {
            wowRemaining = 0.0f;
            wowElapsed = 0.0f;
            switchToView({-2.0f, 2.3f, 23.0f}, {0.0f, 2.5f, -1.0f});
        }
        if (pressed(GLFW_KEY_V, previousV))
        {
            cinematicMode = !cinematicMode;
            if (cinematicMode)
            {
                cinematicTime = 0.0f;
                cinematicBlend = 0.0f;
                cinematicStart = camera;
            }
        }

        if (pressed(GLFW_KEY_SPACE, previousSpace))
        {
            wowRemaining = wowDuration;
            wowElapsed = 0.0f;
            wowStart = camera;
        }
        if (wowRemaining > 0.0f)
        {
            wowRemaining = std::max(0.0f, wowRemaining - deltaTime);
            wowElapsed += deltaTime;
        }
        float wowStrength = std::min(1.0f, std::min(wowElapsed / 0.45f,
                                                    wowRemaining / 0.85f));
        wowStrength = std::max(0.0f, wowStrength);

        if (wowRemaining <= 0.0f && cinematicMode)
        {
            cinematicTime += deltaTime;
            cinematicBlend = smoothStep01(cinematicTime / 1.7f);
            const float orbit = std::max(0.0f, cinematicTime - 1.7f);
            const float theta = orbit * 0.18f;
            const glm::vec3 desiredPosition(
                6.5f * std::cos(theta + 0.9f),
                7.2f + 1.6f * std::sin(orbit * 0.35f),
                17.0f + 6.0f * std::sin(theta + 0.9f));
            const glm::vec3 position = glm::mix(
                cinematicStart.getPosition(), desiredPosition, cinematicBlend);
            const glm::vec3 desiredTarget(0.0f, 2.0f, -1.0f);
            const glm::vec3 desiredFront = glm::normalize(desiredTarget - position);
            glm::vec3 front = glm::mix(cinematicStart.getFront(), desiredFront,
                                       cinematicBlend);
            front = glm::normalize(front);
            pointCameraAt(camera, position, position + front);
        }

        if (wowRemaining > 0.0f)
        {
            const float progress = std::min(1.0f, wowElapsed / wowDuration);
            const float pulse = std::sin(pi * progress) * wowStrength;
            const glm::vec3 start = wowStart.getPosition();
            const glm::vec3 front = wowStart.getFront();
            const glm::vec3 position = start + front * (2.6f * pulse) + glm::vec3(0.0f, 0.7f * pulse, 0.0f);
            const glm::vec3 desiredFront = glm::normalize(
                glm::vec3(0.0f, 2.0f, 0.0f) - position);
            glm::vec3 direction = glm::normalize(glm::mix(front, desiredFront,
                                                          0.22f * pulse));
            pointCameraAt(camera, position, position + direction);
        }
        else if (wowElapsed > 0.0f)
        {
            camera = wowStart;
            wowElapsed = 0.0f;
        }

        showTime += deltaTime * showSpeed;
        if (audio.isPlaying())
        {
            beatTime = audio.getPlaybackTimeSeconds() * musicBPM / 60.0f;

            float beatPhase = beatTime - std::floor(beatTime);

            beatPulse = std::exp(-beatPhase * 6.0f);
        }
        else
        {
            beatPulse = 0.0f;
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        if (!audiencePaused)
            audienceTime += deltaTime * showSpeed;
        ++fpsFrames;
        if (now - fpsStart >= 0.5)
        {
            currentFPS = static_cast<int>(fpsFrames / (now - fpsStart));

            std::ostringstream title;
            title << "Concert Stage | FPS " << currentFPS << " | Audience " << (audienceVisible ? audience.count() : 0)
                  << (audiencePaused ? " paused" : (audienceEnergetic ? " energetic" : " calm"))
                  << " | Light " << lightingMode << " | Speed " << showSpeed << "x"
                  << " | Music " << (audio.isPlaying() ? "ON" : "OFF")
                  << " | Volume " << static_cast<int>(musicVolume * 100.0f) << "%"
                  << (wowRemaining > 0.0f ? " | WOW!" : "")
                  << (cinematicMode ? " | CINEMATIC" : " | MANUAL")
                  << " | C reset / V cinematic / 4-7/F1-F4 views / Space WOW / Esc";
            glfwSetWindowTitle(window, title.str().c_str());
            fpsStart = now;
            fpsFrames = 0;
        }
        float time = showTime;

        for (int i = 0; i < 4; i++)
        {
            float phase = static_cast<float>(i) * 1.57f;

            float angle = std::sin(time * (0.7f + 0.6f * wowStrength) + phase) * (0.45f + 0.22f * wowStrength);

            lights[i].direction = glm::normalize(
                glm::vec3(
                    std::sin(angle),
                    -std::cos(angle),
                    0.0f));

            const float spotlightX[4] = {
                -5.0f, -2.5f, 2.5f, 5.0f};

            glm::vec3 pivot(
                spotlightX[i], 5.5f, -2.5f);

            glm::mat4 rotation = glm::rotate(
                glm::mat4(1.0f),
                angle,
                glm::vec3(0.0f, 0.0f, 1.0f));

            glm::vec3 bulbOffset(0.0f, -0.55f, 0.0f);

            lights[i].position = pivot +
                                 glm::vec3(rotation * glm::vec4(bulbOffset, 1.0f));

            float red = 0.5f + 0.5f *
                                   std::sin(time * 0.6f + phase);

            float green = 0.5f + 0.5f *
                                     std::sin(time * 0.6f + phase + 2.094f);

            float blue = 0.5f + 0.5f *
                                    std::sin(time * 0.6f + phase + 4.188f);

            if (lightingMode == 2)
            {
                lights[i].color = glm::vec3(1.0f, 0.55f + 0.12f * red, 0.16f + 0.10f * blue);
            }
            else if (lightingMode == 3)
            {
                lights[i].color = glm::vec3(0.24f + 0.20f * red, 0.35f + 0.25f * green, 0.9f);
            }
            else
            {
                lights[i].color = glm::vec3(red, green, blue);
            }
            if (wowStrength > 0.0f)
            {
                const glm::vec3 wowPalette[4] = {
                    {1.0f, 0.32f, 0.78f}, {0.25f, 0.85f, 1.0f}, {1.0f, 0.78f, 0.25f}, {0.55f, 0.35f, 1.0f}};
                float breathe = 0.85f + 0.15f * std::sin(time * 3.0f + phase);
                glm::vec3 wowColor = wowPalette[i] * (2.3f * breathe);
                lights[i].color = glm::mix(lights[i].color, wowColor, wowStrength);
                lights[i].color *= (1.0f + beatPulse * 0.65f);
            }
        }

        glm::vec3 targets[3] =
            {
                glm::vec3(-3.0f, 1.0f, -2.0f), // Drummer
                glm::vec3(0.0f, 1.5f, 1.2f),   // Singer
                glm::vec3(3.0f, 1.5f, 0.0f)    // Guitarist
        };

        float followTime = time * 0.25f;

        float targetPosition =
            (std::sin(followTime) + 1.0f) * 1.0f;

        int firstTarget = static_cast<int>(targetPosition);
        int secondTarget = (firstTarget + 1) % 3;

        float blend = targetPosition - firstTarget;
        blend = blend * blend * (3.0f - 2.0f * blend);

        glm::vec3 followTarget = glm::mix(
            targets[firstTarget],
            targets[secondTarget],
            blend);

        lights[4].direction = glm::normalize(
            followTarget - lights[4].position);

        glm::vec3 centralPivot(0.0f, 5.5f, -2.5f);

        float centralAngleZ = std::atan2(
            lights[4].direction.x,
            -lights[4].direction.y);

        float centralAngleX = std::atan2(
            -lights[4].direction.z,
            std::sqrt(
                lights[4].direction.x * lights[4].direction.x +
                lights[4].direction.y * lights[4].direction.y));

        glm::mat4 centralRotation(1.0f);

        centralRotation = glm::rotate(
            centralRotation,
            centralAngleZ,
            glm::vec3(0.0f, 0.0f, 1.0f));

        centralRotation = glm::rotate(
            centralRotation,
            centralAngleX,
            glm::vec3(1.0f, 0.0f, 0.0f));

        glm::vec3 centralBulbOffset(0.0f, -0.55f, 0.0f);

        lights[4].position = centralPivot +
                             glm::vec3(
                                 centralRotation * glm::vec4(centralBulbOffset, 1.0f));

        lights[4].direction = glm::normalize(
            followTarget - lights[4].position);
        lights[4].color = lightingMode == 2   ? glm::vec3(1.0f, 0.82f, 0.45f)
                          : lightingMode == 3 ? glm::vec3(0.45f, 0.75f, 1.0f)
                                              : glm::vec3(1.0f, 0.88f, 0.65f);
        if (wowStrength > 0.0f)
            lights[4].color = glm::mix(lights[4].color,
                                       glm::vec3(2.3f, 2.0f, 1.6f), wowStrength);
        lights[4].color *= (1.0f + beatPulse * 0.65f);

        int spotlightNumbers[4] = {1, 2, 4, 5};

        for (GpuMesh &mesh : gpuMeshes)
        {
            for (int i = 0; i < 4; i++)
            {
                float phase = static_cast<float>(i) * 1.57f;
                float angle = std::sin(time * (0.7f + 0.6f * wowStrength) + phase) * (0.45f + 0.22f * wowStrength);

                animateSpotlightMesh(
                    mesh,
                    spotlightNumbers[i],
                    angle);
            }

            animateCentralSpotlight(mesh, lights[4].direction);
            animatePerformer(mesh, time);
        }

        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(window, &width, &height);

        if (width > 0 && height > 0)
        {
            glViewport(0, 0, width, height);

            glClearColor(0.025f + 0.035f * wowStrength,
                         0.025f + 0.018f * wowStrength,
                         0.08f + 0.07f * wowStrength, 1.0f);

            glClear(
                GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 projection = camera.getProjectionMatrix(
                static_cast<float>(width) / static_cast<float>(height));

            glUseProgram(shaderProgram);

            GLint emissionLocation = glGetUniformLocation(
                shaderProgram,
                "uEmissionColor");

            glUniform3fv(
                cameraPosLocation,
                1,
                glm::value_ptr(camera.getPosition()));

            for (int i = 0; i < 5; i++)
            {

                glm::vec3 worldLightPosition = lights[i].position + stageOffset;

                std::string prefix =
                    "uLights[" + std::to_string(i) + "].";


                glUniform3fv(
                    glGetUniformLocation(
                        shaderProgram,
                        (prefix + "position").c_str()),
                    1,
                    glm::value_ptr(worldLightPosition));

                glUniform3fv(
                    glGetUniformLocation(
                        shaderProgram,
                        (prefix + "direction").c_str()),
                    1,
                    glm::value_ptr(lights[i].direction));

                glm::vec3 activeLightColor =
                        lights[i].color * spotlightBrightness;

                    glUniform3fv(
                        glGetUniformLocation(
                            shaderProgram,
                            (prefix + "color").c_str()),
                        1,
                        glm::value_ptr(activeLightColor));

                float innerAngle = (i == 4) ? 40.0f : 25.0f;
                float outerAngle = (i == 4) ? 50.0f : 35.0f;

                glUniform1f(
                    glGetUniformLocation(
                        shaderProgram,
                        (prefix + "cutOff").c_str()),
                    glm::cos(glm::radians(innerAngle)));

                glUniform1f(
                    glGetUniformLocation(
                        shaderProgram,
                        (prefix + "outerCutOff").c_str()),
                    glm::cos(glm::radians(outerAngle)));
            }

            glUniformMatrix4fv(
                viewLocation,
                1,
                GL_FALSE,
                glm::value_ptr(view));

            glUniformMatrix4fv(
                projectionLocation,
                1,
                GL_FALSE,
                glm::value_ptr(projection));

            for (const GpuMesh &mesh : gpuMeshes)
            {
                bool isEnvironment =
                    mesh.name.find("Club ") == 0 ||
                    mesh.name == "Audience floor";

                glm::mat4 worldModel = isEnvironment
                    ? mesh.model
                    : stageTransform * mesh.model;

                glUniformMatrix4fv(
                    modelLocation,
                    1,
                    GL_FALSE,
                    glm::value_ptr(worldModel));

                glm::vec3 emissionColor(0.0f);

                if (mesh.name.find("Stage LED") == 0)
                {
                    emissionColor = glm::vec3(0.02f, 0.65f, 0.95f);
                }

                int bulbNumbers[5] = {1, 2, 4, 5, 3};

                for (int i = 0; i < 5; i++)
                {
                    std::string bulbName =
                        "Spotlight Bulb " + std::to_string(bulbNumbers[i]);

                    if (mesh.name == bulbName)
                    {
                        emissionColor =
                            lights[i].color *
                            (1.5f + 0.65f * wowStrength) *
                            spotlightBrightness;
                        break;
                    }
                }

                glUniform3fv(
                    emissionLocation,
                    1,
                    glm::value_ptr(emissionColor));


                bool useBrickTexture =
                    (mesh.name.find("Club ") == 0 &&
                    mesh.name.find("Wall") != std::string::npos) ||
                    mesh.name == "Club Entrance Top";

                glUniform1i(
                    glGetUniformLocation(shaderProgram, "uUseTexture"),
                    useBrickTexture ? 1 : 0
                );

                glm::vec2 textureScale(4.0f, 2.0f);

                if (mesh.name == "Club Entrance Top")
                {
                    textureScale = glm::vec2(1.0f, 2.0f);
                }


                glActiveTexture(GL_TEXTURE0);
                glBindTexture(
                    GL_TEXTURE_2D,
                    useBrickTexture ? brickTexture : 0
                );    

                glBindVertexArray(mesh.VAO);

                glDrawElements(
                    GL_TRIANGLES,
                    mesh.indexCount,
                    GL_UNSIGNED_INT,
                    nullptr);
            }

            glUniform1i(
                glGetUniformLocation(shaderProgram, "uUseTexture"),
                GL_FALSE
            );

            if (audienceVisible)
                audience.draw(shaderProgram, audienceTime, audienceEnergetic || wowStrength > 0.2f);


            if (soundWaveProgram != 0 && soundWaveVisibility > 0.01f)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);

                glUseProgram(soundWaveProgram);
                glBindVertexArray(waveVAO);

                GLint mvpLoc = glGetUniformLocation(soundWaveProgram, "uMVP");
                GLint radiusLoc = glGetUniformLocation(soundWaveProgram, "uRadius");
                GLint timeLoc = glGetUniformLocation(soundWaveProgram, "uTime");
                GLint phaseLoc = glGetUniformLocation(soundWaveProgram, "uPhase");
                GLint colorOffsetLoc = glGetUniformLocation(soundWaveProgram, "uColorOffset");
                GLint alphaLoc = glGetUniformLocation(soundWaveProgram, "uAlpha");

                glUniform1f(timeLoc, soundWaveTime);

                for (int speaker = 0; speaker < 2; ++speaker)
                {
                    float x = speaker == 0 ? -7.0f : 7.0f;


                    for (int i = 0; i < 4; ++i)
                    {
                        float phase = std::fmod(
                            soundWaveTime * 0.8f + i * 0.25f,
                            1.0f
                        );

                        glUniform1f(phaseLoc, i * 0.35f + speaker * 0.2f);
                        float colorProgress = soundWaveTime * (5.0f / 5.0f);

                        float waveColor = colorProgress - phase * 5.0f;

                        glUniform1f(colorOffsetLoc, waveColor);

                        float radius = 0.45f + phase * 2.0f;
                        float z = 1.7f + phase * 3.5f;

                        float alpha = (1.0f - phase) *
                                    0.65f * soundWaveVisibility;

                        glm::mat4 model(1.0f);
                        model = glm::translate(
                            model,
                            glm::vec3(x, 3.2f, z)
                        );

                        glm::mat4 mvp = projection * view * model;

                        glUniformMatrix4fv(
                            mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp)
                        );

                        glUniform1f(radiusLoc, radius);
                        glUniform1f(alphaLoc, alpha);

                        glLineWidth(2.0f);
                        glDrawArrays(GL_LINE_LOOP, 0, waveSegments);
                    }
                }

                glBindVertexArray(0);
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }



            glBindVertexArray(0);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);

            glUseProgram(glowShaderProgram);

            glUniformMatrix4fv(
                glGetUniformLocation(glowShaderProgram, "uView"),
                1,
                GL_FALSE,
                glm::value_ptr(view));

            glUniformMatrix4fv(
                glGetUniformLocation(glowShaderProgram, "uProjection"),
                1,
                GL_FALSE,
                glm::value_ptr(projection));

            glUniform1f(
                glGetUniformLocation(glowShaderProgram, "uGlowSize"),
                0.65f + 1.0f * wowStrength);

            glBindVertexArray(glowVAO);

            for (int i = 0; i < 5; i++)
            {
                glm::vec3 worldLightPosition =
                    lights[i].position + stageOffset;

                glUniform3fv(
                    glGetUniformLocation(
                        glowShaderProgram,
                        "uGlowPosition"),
                    1,
                    glm::value_ptr(worldLightPosition));

                glm::vec3 glowColor =
                    lights[i].color * spotlightBrightness;

                glUniform3fv(
                    glGetUniformLocation(
                        glowShaderProgram,
                        "uGlowColor"),
                    1,
                    glm::value_ptr(glowColor));

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            glUseProgram(ledGlowShaderProgram);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            glUniformMatrix4fv(
                glGetUniformLocation(ledGlowShaderProgram, "uView"),
                1, GL_FALSE, glm::value_ptr(view));

            glUniformMatrix4fv(
                glGetUniformLocation(ledGlowShaderProgram, "uProjection"),
                1, GL_FALSE, glm::value_ptr(projection));

            glm::vec3 ledGlowColor(0.02f, 0.65f, 0.95f);

            glUniform3fv(
                glGetUniformLocation(ledGlowShaderProgram, "uColor"),
                1, glm::value_ptr(ledGlowColor));

            float ledIntensity = 0.85f + wowStrength * 0.3f;

            glUniform1f(
                glGetUniformLocation(ledGlowShaderProgram, "uIntensity"),
                ledIntensity);

            glBindVertexArray(glowVAO);

            auto drawLedGlow = [&](glm::vec3 center,
                                glm::vec3 axis,
                                float length,
                                float height)
            {
                center += stageOffset;

                glUniform3fv(
                    glGetUniformLocation(ledGlowShaderProgram, "uCenter"),
                    1, glm::value_ptr(center));

                glUniform3fv(
                    glGetUniformLocation(ledGlowShaderProgram, "uAxis"),
                    1, glm::value_ptr(axis));

                glUniform1f(
                    glGetUniformLocation(ledGlowShaderProgram, "uLength"),
                    length);

                glUniform1f(
                    glGetUniformLocation(ledGlowShaderProgram, "uHeight"),
                    height);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            };

            drawLedGlow(
                {0.0f, -0.18f, 5.22f},
                {1.0f, 0.0f, 0.0f},
                16.1f,
                0.65f);

            drawLedGlow(
                {0.0f, -1.32f, 5.22f},
                {1.0f, 0.0f, 0.0f},
                16.1f,
                0.50f);

            drawLedGlow(
                {-8.13f, -0.18f, 0.0f},
                {0.0f, 0.0f, 1.0f},
                10.1f,
                0.65f);

            drawLedGlow(
                {8.13f, -0.18f, 0.0f},
                {0.0f, 0.0f, 1.0f},
                10.1f,
                0.65f);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            confetti.render(view, projection);
        }

        if (textRenderer.isInitialized() && width > 0 && height > 0)
        {
            textRenderer.renderText(
                "FPS: " + std::to_string(currentFPS),
                20.0f,
                static_cast<float>(height) - 38.0f,
                1.0f,
                glm::vec3(1.0f, 1.0f, 1.0f),
                width,
                height);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    audience.release();
    deleteGpuMeshes(gpuMeshes);
    audio.shutdown();

    confetti.cleanup();
    textRenderer.cleanup();

    glDeleteBuffers(1, &glowVBO);
    glDeleteTextures(1, &brickTexture);
    glDeleteVertexArrays(1, &glowVAO);

    glDeleteProgram(glowShaderProgram);
    glDeleteProgram(ledGlowShaderProgram);
    glDeleteProgram(shaderProgram);

    glDeleteVertexArrays(1, &waveVAO);
    glDeleteBuffers(1, &waveVBO);
    glDeleteProgram(soundWaveProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Concert Stage closed successfully!" << std::endl;

    return 0;
}
