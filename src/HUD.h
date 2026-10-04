#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct GLFWwindow;
class Camera;
class CelestialBody;
class SimulationClock;
class PhysicalObserver;
class SpaceRocket;
class ObserverGravitySystem;
class NamedStarField;

class HUD
{
public:
    void UpdateToggle(GLFWwindow* window, double& lastX, double& lastY);
    void UpdateNavigation(Camera& camera, const std::vector<CelestialBody>& bodies);
    void Draw(Camera& camera, std::vector<CelestialBody>& bodies, const NamedStarField& namedStarField, SimulationClock& simulationClock, PhysicalObserver& observer, SpaceRocket& rocket, const ObserverGravitySystem& observerGravitySystem, const glm::mat4& view, const glm::mat4& projection, int width, int height);

    bool IsOpen() const;
    bool IsAberrationEnabled() const;
    int GetSelectedTargetIndex(const std::vector<CelestialBody>& bodies) const;

private:
    bool open = false;
    bool tabWasPressed = false;
    bool cWasPressed = false;
    bool bodyCreatorOpen = false;
    bool showNameTags = true;
    bool autoSpeed = false;
    bool aberrationEnabled = true;

    char spawnName[64] = "New Body";
    int spawnType = 0;
    double spawnMassSolarMasses = 3.00348959632e-6;
    double spawnRadiusKm = 6371.0;
    double spawnDistanceMeters = 149597870700.0;
    double spawnForwardKmS = 0.0;
    double spawnRightKmS = 0.0;
    double spawnUpKmS = 0.0;
    bool spawnShowNameTag = true;
    bool spawnInheritTargetVelocity = false;
    bool spawnCircularBinary = false;

    int selectedTarget = 1;
    int selectedNamedStar = 0;

    bool WorldToScreen(const CelestialBody& body, const Camera& camera, const glm::mat4& view, const glm::mat4& projection, int width, int height, glm::vec2& screen) const;
    void DrawNameTags(const Camera& camera, const std::vector<CelestialBody>& bodies, const glm::mat4& view, const glm::mat4& projection, int width, int height) const;
    void DrawGalacticCenterTag(const Camera& camera, const glm::mat4& view, const glm::mat4& projection, int width, int height) const;
    void DrawNamedStarTag(const Camera& camera, const NamedStarField& namedStarField, const glm::mat4& view, const glm::mat4& projection, int width, int height) const;
    void DrawNamedStarSelector(Camera& camera, const NamedStarField& namedStarField);
    void DrawObserverOverlay(const PhysicalObserver& observer, const SimulationClock& simulationClock, const ObserverGravitySystem& observerGravitySystem, const std::vector<CelestialBody>& bodies) const;
    void DrawRocketOverlay(const SpaceRocket& rocket, const SimulationClock& simulationClock) const;
    void DrawCrosshair(int width, int height) const;
    void DrawTargetSelector(const Camera& camera, const std::vector<CelestialBody>& bodies);
    void DrawBodyCreator(Camera& camera, std::vector<CelestialBody>& bodies, int width, int height);
    std::string FormatDistance(double meters) const;
    std::string FormatTime(double seconds) const;
    std::string FormatRelativisticSpeed(double beta, double log10OneMinusBeta) const;
    std::string FormatMass(double mass) const;
    std::string FormatNumber(double value, int decimals = 3) const;
    std::string FormatScale(double scale) const;
};
