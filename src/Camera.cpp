#include "Camera.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

glm::dvec3 Camera::Front() const
{
    double yaw = glm::radians(Yaw);
    double pitch = glm::radians(Pitch);

    glm::dvec3 front;
    front.x = std::cos(yaw) * std::cos(pitch);
    front.y = std::sin(pitch);
    front.z = std::sin(yaw) * std::cos(pitch);

    return glm::normalize(front);
}

glm::dvec3 Camera::Right() const
{
    return glm::normalize(glm::cross(Front(), glm::dvec3(0.0, 1.0, 0.0)));
}

glm::mat4 Camera::GetViewRotationMatrix() const
{
    glm::dmat4 view = glm::lookAt(glm::dvec3(0.0), Front(), glm::dvec3(0.0, 1.0, 0.0));
    return glm::mat4(view);
}

void Camera::LookAt(const glm::dvec3& target)
{
    glm::dvec3 delta = target - Position;
    double distance = glm::length(delta);

    if (distance <= 0.0)
        return;

    glm::dvec3 direction = delta / distance;
    Pitch = glm::degrees(std::asin(std::clamp(direction.y, -1.0, 1.0)));
    Yaw = glm::degrees(std::atan2(direction.z, direction.x));
}

void Camera::ProcessKeyboard(double deltaTime, bool forward, bool backward, bool left, bool right, bool up, bool down)
{
    double velocity = Speed * deltaTime;

    if (forward)
        Position += Front() * velocity;

    if (backward)
        Position -= Front() * velocity;

    if (left)
        Position -= Right() * velocity;

    if (right)
        Position += Right() * velocity;

    if (up)
        Position.y += velocity;

    if (down)
        Position.y -= velocity;
}

void Camera::ProcessMouse(double xOffset, double yOffset)
{
    Yaw += xOffset * Sensitivity;
    Pitch += yOffset * Sensitivity;
    Pitch = std::clamp(Pitch, -89.0, 89.0);
}
