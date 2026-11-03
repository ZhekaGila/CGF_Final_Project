// Camera.cpp
// Participant 4 — Camera and Controls

#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Camera::Camera(glm::vec3 position, float yaw, float pitch)
    : m_position(position),
      m_front(0.0f, 0.0f, -1.0f),
      m_up(0.0f, 1.0f, 0.0f),
      m_right(1.0f, 0.0f, 0.0f),
      m_worldUp(0.0f, 1.0f, 0.0f),
      m_yaw(yaw),
      m_pitch(pitch)
{
    updateVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    // lookAt(eye, target, up): camera at m_position, looking one step forward
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::processKeyboard(Movement direction, float deltaTime, bool fast)
{
    // deltaTime keeps the speed the same on fast and slow computers
    float velocity = m_movementSpeed * deltaTime;
    if (fast)
    {
        velocity *= 3.0f;
    }

    switch (direction)
    {
    case Movement::Forward:  m_position += m_front * velocity; break;
    case Movement::Backward: m_position -= m_front * velocity; break;
    case Movement::Left:     m_position -= m_right * velocity; break;
    case Movement::Right:    m_position += m_right * velocity; break;
    case Movement::Up:       m_position += m_worldUp * velocity; break;
    case Movement::Down:     m_position -= m_worldUp * velocity; break;
    }
}

void Camera::processMouseMovement(float xOffset, float yOffset)
{
    m_yaw += xOffset * m_mouseSensitivity;
    m_pitch += yOffset * m_mouseSensitivity;

    // Do not let the camera flip over the top or bottom
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    updateVectors();
}

void Camera::processMouseScroll(float yOffset)
{
    // Scrolling up makes the field of view smaller = zoom in
    m_fov = std::clamp(m_fov - yOffset, 20.0f, 75.0f);
}

void Camera::updateVectors()
{
    // Convert yaw and pitch angles into a direction vector
    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    // Right and up vectors are perpendicular to the front vector
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}