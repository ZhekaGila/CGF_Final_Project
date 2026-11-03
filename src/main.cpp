#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <iostream>

#include "Camera.h" // [Camera] Participant 4

// ---------------------------------------------------------------------------
// [Camera] Participant 4 — camera object and mouse state
// ---------------------------------------------------------------------------
static Camera g_camera;               // the scene camera (other parts use its matrices)
static float g_lastMouseX = 640.0f;   // last mouse position, starts at window center
static float g_lastMouseY = 360.0f;
static bool g_firstMouse = true;      // avoids a jump on the first mouse event

static void mouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    (void)window;

    float x = static_cast<float>(xPos);
    float y = static_cast<float>(yPos);

    if (g_firstMouse)
    {
        g_lastMouseX = x;
        g_lastMouseY = y;
        g_firstMouse = false;
    }

    float xOffset = x - g_lastMouseX;
    float yOffset = g_lastMouseY - y; // reversed: screen Y goes down, pitch goes up
    g_lastMouseX = x;
    g_lastMouseY = y;

    g_camera.processMouseMovement(xOffset, yOffset);
}

static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    (void)window;
    (void)xOffset;
    g_camera.processMouseScroll(static_cast<float>(yOffset));
}

static void processCameraInput(GLFWwindow* window, float deltaTime)
{
    // Esc closes the program (the mouse cursor is hidden while it runs)
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    bool fast = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_camera.processKeyboard(Camera::Movement::Forward, deltaTime, fast);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        g_camera.processKeyboard(Camera::Movement::Backward, deltaTime, fast);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        g_camera.processKeyboard(Camera::Movement::Left, deltaTime, fast);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        g_camera.processKeyboard(Camera::Movement::Right, deltaTime, fast);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        g_camera.processKeyboard(Camera::Movement::Up, deltaTime, fast);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        g_camera.processKeyboard(Camera::Movement::Down, deltaTime, fast);

    // C prints the camera position (for testing while the scene is still empty)
    static bool cWasPressed = false;
    bool cPressed = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
    if (cPressed && !cWasPressed)
    {
        glm::vec3 p = g_camera.getPosition();
        std::cout << "Camera position: " << p.x << ", " << p.y << ", " << p.z
                  << " | FOV: " << g_camera.getFov() << std::endl;
    }
    cWasPressed = cPressed;
}
// ---------------------------------------------------------------------------

int main()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // Request OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(
        1280,
        720,
        "Concert Stage",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Activate OpenGL context
    glfwMakeContextCurrent(window);

    // Load OpenGL functions using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Enable V-Sync
    glfwSwapInterval(1);

    // [Camera] Mouse look: hide the cursor and listen to mouse movement and scroll
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Test GLM
    glm::vec3 stagePosition(0.0f, 0.0f, 0.0f);

    std::cout << "Concert Stage started!" << std::endl;
    std::cout << "GLM test: "
              << stagePosition.x << ", "
              << stagePosition.y << ", "
              << stagePosition.z << std::endl;
    std::cout << "Controls: WASD move, Q/E down/up, Shift fast, mouse look, "
                 "scroll zoom, C print camera, Esc quit" << std::endl;

    float lastFrameTime = static_cast<float>(glfwGetTime()); // [Camera]

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        // [Camera] Time between frames, so movement speed does not depend on FPS
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        processCameraInput(window, deltaTime); // [Camera]

        int width, height;

        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        // [Camera] Matrices for the shaders (used when the scene is drawn)
        float aspectRatio = (height > 0) ? static_cast<float>(width) / height : 1.0f;
        glm::mat4 view = g_camera.getViewMatrix();
        glm::mat4 projection = g_camera.getProjectionMatrix(aspectRatio);
        (void)view;
        (void)projection;

        // Clear screen
        glClearColor(0.03f, 0.03f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Swap buffers and process events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}