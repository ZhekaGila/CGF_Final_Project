#pragma once

// Camera.h
// Participant 4 — Camera and Controls
//
// A first-person "fly" camera for the concert stage.
//   W / A / S / D  - move forward / left / back / right
//   Q / E          - move down / up
//   Left Shift     - move faster
//   Mouse          - look around
//   Mouse wheel    - zoom (change field of view)
//
// Other team members only need two functions:
//   getViewMatrix()        - where the camera is and where it looks
//   getProjectionMatrix()  - perspective (field of view, near/far planes)
// They pass these matrices to their shaders as "view" and "projection".

#include <glm/glm.hpp>

class Camera
{
public:
    // Directions the camera can move in (used by processKeyboard)
    enum class Movement
    {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down
    };

    // position - where the camera starts
    // yaw      - left/right angle in degrees (-90 = looking along -Z)
    // pitch    - up/down angle in degrees (0 = straight ahead)
    explicit Camera(glm::vec3 position = glm::vec3(0.0f, 2.0f, 8.0f),
                    float yaw = -90.0f,
                    float pitch = -10.0f);

    // Matrices for the shaders
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Input
    void processKeyboard(Movement direction, float deltaTime, bool fast = false);
    void processMouseMovement(float xOffset, float yOffset);
    void processMouseScroll(float yOffset);

    // Getters (useful for lighting and debugging)
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getFront() const { return m_front; }
    float getFov() const { return m_fov; }

    // Settings
    void setMovementSpeed(float speed) { m_movementSpeed = speed; }
    void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }

private:
    // Recalculates front/right/up vectors from yaw and pitch
    void updateVectors();

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    float m_movementSpeed = 5.0f;    // units per second
    float m_mouseSensitivity = 0.1f; // degrees per pixel
    float m_fov = 45.0f;             // field of view in degrees

    float m_nearPlane = 0.1f;
    float m_farPlane = 200.0f;
};