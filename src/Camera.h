#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    glm::dvec3 Position = glm::dvec3(0.0, 0.0, 3.0e7);

    double Yaw = -90.0;
    double Pitch = 0.0;
    double Speed = 5.0e6;
    double Sensitivity = 0.1;

    glm::dvec3 Front() const;
    glm::dvec3 Right() const;
    glm::mat4 GetViewRotationMatrix() const;

    void LookAt(const glm::dvec3& target);
    void ProcessKeyboard(double deltaTime, bool forward, bool backward, bool left, bool right, bool up, bool down);
    void ProcessMouse(double xOffset, double yOffset);
};
