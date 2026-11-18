
#pragma once

// Participant 4 — First-person and cinematic camera control.
#include <glm/glm.hpp>

class Camera
{
public:
    enum class Movement
    {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down
    };

    explicit Camera(glm::vec3 position = glm::vec3(0.0f, 2.0f, 8.0f),
                    float yaw = -90.0f,
                    float pitch = -10.0f);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    void processKeyboard(Movement direction, float deltaTime, bool fast = false);
    void processMouseMovement(float xOffset, float yOffset);
    void processMouseScroll(float yOffset);

    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getFront() const { return m_front; }
    float getFov() const { return m_fov; }

    void setMovementSpeed(float speed) { m_movementSpeed = speed; }
    void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }

    // Update camera pose without recreating the object or resetting its FOV.
    // Used by camera presets, cinematic mode and Wow moment.
    void setPose(const glm::vec3& position, float yaw, float pitch)
    {
        m_position = position;
        m_yaw = yaw;
        m_pitch = pitch;
        updateVectors();
    }

private:
    void updateVectors();

    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;

    float m_yaw;
    float m_pitch;

    float m_movementSpeed = 5.0f;
    float m_mouseSensitivity = 0.1f;
    float m_fov = 45.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 200.0f;
};
