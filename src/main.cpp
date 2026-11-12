
#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "geometry/StageGeometry.h"

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

// ============================================================
// SHADERS
// ============================================================

std::string loadShaderSource(const std::string& filePath)
{
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: " << filePath <<std::endl;

        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return buffer.str();
}

// ============================================================
// SHADER COMPILATION
// ============================================================

GLuint compileShader(GLenum type, const char* source)
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


// ============================================================
// SHADER PROGRAM
// ============================================================

GLuint createShaderProgram()
{

    std::string vertexSource = loadShaderSource("shaders/stage.vert");
    
    std::string fragmentSource = loadShaderSource("shaders/stage.frag");

    if(vertexSource.empty() || fragmentSource.empty())
    {
        return 0;
    }

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource.c_str());

    if(vertexShader == 0)
        return 0;

    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str());

    if( fragmentShader == 0)
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


// ============================================================
// GPU MESH
// ============================================================

struct GpuMesh
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    GLsizei indexCount = 0;

    glm::mat4 model = glm::mat4(1.0f);
};


// ============================================================
// UPLOAD MESH TO GPU
// ============================================================

GpuMesh uploadMesh(const cgf::ScenePart& part)
{
    GpuMesh gpuMesh;

    gpuMesh.model = part.transform;

    gpuMesh.indexCount =
        static_cast<GLsizei>(part.mesh.indices.size());

    glGenVertexArrays(1, &gpuMesh.VAO);
    glGenBuffers(1, &gpuMesh.VBO);
    glGenBuffers(1, &gpuMesh.EBO);

    glBindVertexArray(gpuMesh.VAO);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            part.mesh.vertices.size() * sizeof(cgf::Vertex)
        ),
        part.mesh.vertices.data(),
        GL_STATIC_DRAW
    );

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            part.mesh.indices.size() * sizeof(std::uint32_t)
        ),
        part.mesh.indices.data(),
        GL_STATIC_DRAW
    );

    // Position: location 0
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(cgf::Vertex),
        reinterpret_cast<void*>(offsetof(cgf::Vertex, position))
    );

    glEnableVertexAttribArray(0);

    // Normal: location 1
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(cgf::Vertex),
        reinterpret_cast<void*>(offsetof(cgf::Vertex, normal))
    );

    glEnableVertexAttribArray(1);

    // UV: location 2
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(cgf::Vertex),
        reinterpret_cast<void*>(offsetof(cgf::Vertex, uv))
    );

    glEnableVertexAttribArray(2);

    // Color: location 3
    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(cgf::Vertex),
        reinterpret_cast<void*>(offsetof(cgf::Vertex, color))
    );

    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    return gpuMesh;
}


// ============================================================
// DELETE GPU RESOURCES
// ============================================================

void deleteGpuMeshes(std::vector<GpuMesh>& meshes)
{
    for (GpuMesh& mesh : meshes)
    {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
    }

    meshes.clear();
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    // --------------------------------------------------------
    // 1. INITIALIZE GLFW
    // --------------------------------------------------------

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // Request OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_DEPTH_BITS, 24);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // --------------------------------------------------------
    // 2. CREATE WINDOW
    // --------------------------------------------------------

    GLFWwindow* window = glfwCreateWindow(
        1280,
        720,
        "Concert Stage - 3D Scene",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;

        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    // --------------------------------------------------------
    // 3. INITIALIZE GLAD
    // --------------------------------------------------------

    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(glfwGetProcAddress)
    ))
    {
        std::cerr << "Failed to initialize GLAD!" << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    std::cout << "OpenGL initialized successfully!" << std::endl;

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // --------------------------------------------------------
    // 4. CREATE SHADER PROGRAM
    // --------------------------------------------------------

    GLuint shaderProgram = createShaderProgram();

    if (shaderProgram == 0)
    {
        std::cerr << "Could not create shader program!" << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    GLint modelLocation =
        glGetUniformLocation(shaderProgram, "uModel");

    GLint viewLocation =
        glGetUniformLocation(shaderProgram, "uView");

    GLint projectionLocation =
        glGetUniformLocation(shaderProgram, "uProjection");

    GLint cameraPosLocation = glGetUniformLocation(shaderProgram, "uCameraPos"); 

    // --------------------------------------------------------
    // 5. CREATE STAGE GEOMETRY
    // --------------------------------------------------------

    std::vector<cgf::ScenePart> scene = cgf::createStageGeometry();

    std::cout << "Stage geometry created!" << std::endl;
    std::cout << "Number of scene objects: "
              << scene.size() << std::endl;

    // --------------------------------------------------------
    // 6. UPLOAD GEOMETRY TO GPU
    // --------------------------------------------------------

    std::vector<GpuMesh> gpuMeshes;

    gpuMeshes.reserve(scene.size());

    for (const cgf::ScenePart& part : scene)
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

    // --------------------------------------------------------
    // 7. CAMERA
    // --------------------------------------------------------

    glm::vec3 cameraPosition(12.0f, 9.0f, 18.0f);
    glm::vec3 cameraTarget(0.0f, 2.0f, 0.0f);

    glm::mat4 view = glm::lookAt(
        cameraPosition,
        cameraTarget,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // --------------------------------------------------------
    // 8. RENDERING LOOP
    // --------------------------------------------------------

    while (!glfwWindowShouldClose(window))
    {
        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(window, &width, &height);

        if (width > 0 && height > 0)
        {
            glViewport(0, 0, width, height);

            // Clear screen and depth buffer
            glClearColor(0.025f, 0.025f, 0.08f, 1.0f);

            glClear(
                GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT
            );

            // Perspective projection
            glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),
                static_cast<float>(width) /
                    static_cast<float>(height),
                0.1f,
                100.0f
            );

            // Activate shader
            glUseProgram(shaderProgram);

            glUniform3fv(
                cameraPosLocation,
                1,
                glm::value_ptr(cameraPosition)
            );

            glUniformMatrix4fv(
                viewLocation,
                1,
                GL_FALSE,
                glm::value_ptr(view)
            );

            glUniformMatrix4fv(
                projectionLocation,
                1,
                GL_FALSE,
                glm::value_ptr(projection)
            );

            // Draw all scene objects
            for (const GpuMesh& mesh : gpuMeshes)
            {
                glUniformMatrix4fv(
                    modelLocation,
                    1,
                    GL_FALSE,
                    glm::value_ptr(mesh.model)
                );

                glBindVertexArray(mesh.VAO);

                glDrawElements(
                    GL_TRIANGLES,
                    mesh.indexCount,
                    GL_UNSIGNED_INT,
                    nullptr
                );
            }

            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --------------------------------------------------------
    // 9. CLEANUP
    // --------------------------------------------------------

    deleteGpuMeshes(gpuMeshes);

    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Concert Stage closed successfully!" << std::endl;

    return 0;
}
