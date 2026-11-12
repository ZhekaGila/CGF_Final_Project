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
#include <cmath>

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
        vertexSource.c_str()
    );

    if (vertexShader == 0)
        return 0;

    GLuint fragmentShader = compileShader(
        GL_FRAGMENT_SHADER,
        fragmentSource.c_str()
    );

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
            program, 1024, nullptr, infoLog
        );

        std::cerr << "Glow shader linking failed: "
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

    std::string name;
    glm::mat4 originalModel = glm::mat4(1.0f);
};


// ============================================================
// UPLOAD MESH TO GPU
// ============================================================

GpuMesh uploadMesh(const cgf::ScenePart& part)
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


void animateSpotlightMesh(
    GpuMesh& mesh,
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
        parentTransform, pivot
    );

    parentTransform = glm::rotate(
        parentTransform,
        angle,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    parentTransform = glm::translate(
        parentTransform, -pivot
    );

    mesh.model = parentTransform * mesh.originalModel;
}


void animateCentralSpotlight(
    GpuMesh& mesh,
    const glm::vec3& direction)
{
    if (mesh.name != "Spotlight Fixture 3" &&
        mesh.name != "Spotlight Bulb 3")
    {
        return;
    }

    // Central spotlight mounting point
    glm::vec3 pivot(0.0f, 5.5f, -2.5f);

    // Calculate rotation angles from light direction
    float angleZ = std::atan2(
        direction.x,
        -direction.y
    );

    float angleX = std::atan2(
        -direction.z,
        std::sqrt(
            direction.x * direction.x +
            direction.y * direction.y
        )
    );

    glm::mat4 parentTransform(1.0f);

    parentTransform = glm::translate(
        parentTransform,
        pivot
    );

    parentTransform = glm::rotate(
        parentTransform,
        angleZ,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    parentTransform = glm::rotate(
        parentTransform,
        angleX,
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    parentTransform = glm::translate(
        parentTransform,
        -pivot
    );

    mesh.model = parentTransform * mesh.originalModel;
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

struct SpotLight
{
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
};

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

    GLuint glowShaderProgram = createGlowShaderProgram();

    if (glowShaderProgram == 0)
    {
        std::cerr << "Could not create glow shader program!" << std::endl;
        return -1;
    }

    float glowVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,

        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

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
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(float),
        nullptr
    );

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



    SpotLight lights[5] =
    {
        {
            glm::vec3(-5.0f, 4.95f, -2.5f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 0.1f, 0.1f)
        },

        {
            glm::vec3(-2.5f, 4.95f, -2.5f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.1f, 0.3f, 1.0f)
        },

        {
            glm::vec3(2.5f, 4.95f, -2.5f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.1f, 1.0f, 0.2f)
        },

        {
            glm::vec3(5.0f, 4.95f, -2.5f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.8f, 0.1f, 1.0f)
        },

        {
            glm::vec3(0.0f, 4.95f, -2.5f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(1.0f, 0.88f, 0.65f)
        }
    };




    // --------------------------------------------------------
    // 8. RENDERING LOOP
    // --------------------------------------------------------

    while (!glfwWindowShouldClose(window))
    {


    float time = static_cast<float>(glfwGetTime());

    for (int i = 0; i < 4; i++)
    {
        float phase = static_cast<float>(i) * 1.57f;

        float angle = std::sin(time * 0.7f + phase) * 0.45f;

        lights[i].direction = glm::normalize(
            glm::vec3(
                std::sin(angle),
                -std::cos(angle),
                0.0f
            )
        );

        const float spotlightX[4] = {
            -5.0f, -2.5f, 2.5f, 5.0f
        };

        glm::vec3 pivot(
            spotlightX[i], 5.5f, -2.5f
        );

        glm::mat4 rotation = glm::rotate(
            glm::mat4(1.0f),
            angle,
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        glm::vec3 bulbOffset(0.0f, -0.55f, 0.0f);

        lights[i].position = pivot +
            glm::vec3(rotation * glm::vec4(bulbOffset, 1.0f));


        float red = 0.5f + 0.5f *
            std::sin(time * 0.6f + phase);

        float green = 0.5f + 0.5f *
            std::sin(time * 0.6f + phase + 2.094f);

        float blue = 0.5f + 0.5f *
            std::sin(time * 0.6f + phase + 4.188f);

        lights[i].color = glm::vec3(
            red,
            green,
            blue
        );
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
            blend
        );

        lights[4].direction = glm::normalize(
            followTarget - lights[4].position
        );



        glm::vec3 centralPivot(0.0f, 5.5f, -2.5f);

        float centralAngleZ = std::atan2(
            lights[4].direction.x,
            -lights[4].direction.y
        );

        float centralAngleX = std::atan2(
            -lights[4].direction.z,
            std::sqrt(
                lights[4].direction.x * lights[4].direction.x +
                lights[4].direction.y * lights[4].direction.y
            )
        );

        glm::mat4 centralRotation(1.0f);

        centralRotation = glm::rotate(
            centralRotation,
            centralAngleZ,
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        centralRotation = glm::rotate(
            centralRotation,
            centralAngleX,
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        glm::vec3 centralBulbOffset(0.0f, -0.55f, 0.0f);

        lights[4].position = centralPivot +
            glm::vec3(
                centralRotation * glm::vec4(centralBulbOffset, 1.0f)
            );

        lights[4].direction = glm::normalize(
            followTarget - lights[4].position
        );


        int spotlightNumbers[4] = {1, 2, 4, 5};

        for (GpuMesh& mesh : gpuMeshes)
        {
            for (int i = 0; i < 4; i++)
            {
                float phase = static_cast<float>(i) * 1.57f;
                float angle = std::sin(time * 0.7f + phase) * 0.45f;

                animateSpotlightMesh(
                    mesh,
                    spotlightNumbers[i],
                    angle
                );
            }

            animateCentralSpotlight(mesh, lights[4].direction);
        }

        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(window, &width, &height);

        if (width > 0 && height > 0)
        {
            glViewport(0, 0, width, height);

            glClearColor(0.025f, 0.025f, 0.08f, 1.0f);

            glClear(
                GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT
            );

            glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),
                static_cast<float>(width) /
                    static_cast<float>(height),
                0.1f,
                100.0f
            );

            glUseProgram(shaderProgram);

            GLint emissionLocation = glGetUniformLocation(
                shaderProgram,
                "uEmissionColor"
            );

            glUniform3fv(
                cameraPosLocation,
                1,
                glm::value_ptr(cameraPosition)
            );

            
            for (int i = 0; i < 5; i++)
            {
                std::string prefix =
                    "uLights[" + std::to_string(i) + "].";



                glUniform3fv(
                    glGetUniformLocation(
                        shaderProgram,
                        (prefix + "position").c_str()
                    ),
                    1,
                    glm::value_ptr(lights[i].position)
                );

                glUniform3fv(
                    glGetUniformLocation(
                        shaderProgram,
                        (prefix + "direction").c_str()
                    ),
                    1,
                    glm::value_ptr(lights[i].direction)
                );

                glUniform3fv(
                    glGetUniformLocation(
                        shaderProgram,
                        (prefix + "color").c_str()
                    ),
                    1,
                    glm::value_ptr(lights[i].color)
                );

                float innerAngle = (i == 4) ? 40.0f : 25.0f;
                float outerAngle = (i == 4) ? 50.0f : 35.0f;

                glUniform1f(
                    glGetUniformLocation(
                        shaderProgram,
                        (prefix + "cutOff").c_str()
                    ),
                    glm::cos(glm::radians(innerAngle))
                );

                glUniform1f(
                    glGetUniformLocation(
                        shaderProgram,
                        (prefix + "outerCutOff").c_str()
                    ),
                    glm::cos(glm::radians(outerAngle))
                );
            }

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

            for (const GpuMesh& mesh : gpuMeshes)
            {
                glUniformMatrix4fv(
                    modelLocation,
                    1,
                    GL_FALSE,
                    glm::value_ptr(mesh.model)
                );


                glm::vec3 emissionColor(0.0f);

                int bulbNumbers[5] = {1, 2, 4, 5, 3};

                for (int i = 0; i < 5; i++)
                {
                    std::string bulbName =
                        "Spotlight Bulb " + std::to_string(bulbNumbers[i]);

                    if (mesh.name == bulbName)
                    {
                        emissionColor = lights[i].color * 1.5f;
                        break;
                    }
                }

                glUniform3fv(
                    emissionLocation,
                    1,
                    glm::value_ptr(emissionColor)
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
        
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            glUseProgram(glowShaderProgram);

            glUniformMatrix4fv(
                glGetUniformLocation(glowShaderProgram, "uView"),
                1,
                GL_FALSE,
                glm::value_ptr(view)
            );

            glUniformMatrix4fv(
                glGetUniformLocation(glowShaderProgram, "uProjection"),
                1,
                GL_FALSE,
                glm::value_ptr(projection)
            );

            glUniform1f(
                glGetUniformLocation(glowShaderProgram, "uGlowSize"),
                0.65f
            );

            glBindVertexArray(glowVAO);

            for (int i = 0; i < 5; i++)
            {
                glUniform3fv(
                    glGetUniformLocation(
                        glowShaderProgram,
                        "uGlowPosition"
                    ),
                    1,
                    glm::value_ptr(lights[i].position)
                );

                glUniform3fv(
                    glGetUniformLocation(
                        glowShaderProgram,
                        "uGlowColor"
                    ),
                    1,
                    glm::value_ptr(lights[i].color)
                );

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            glBindVertexArray(0);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);


        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --------------------------------------------------------
    // 9. CLEANUP
    // --------------------------------------------------------

    deleteGpuMeshes(gpuMeshes);

    glDeleteBuffers(1, &glowVBO);
    glDeleteVertexArrays(1, &glowVAO);

    glDeleteProgram(glowShaderProgram);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "Concert Stage closed successfully!" << std::endl;

    return 0;
}
