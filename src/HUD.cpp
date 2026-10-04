#include "HUD.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "Camera.h"
#include "CelestialBody.h"
#include "PhysicsConstants.h"
#include "GalacticConstants.h"
#include "GalacticFrame.h"
#include "RenderConstants.h"
#include "SimulationClock.h"
#include "PhysicalObserver.h"
#include "SpaceRocket.h"
#include "ObserverGravitySystem.h"
#include "NamedStarField.h"
#include "AndromedaModel.h"

namespace
{
    std::string FormatTinyFixed(double value, int extraDigits = 3)
    {
        if (!std::isfinite(value))
            return "n/a";

        value = std::abs(value);

        if (value == 0.0)
            return "0";

        int decimals = static_cast<int>(std::ceil(-std::log10(value))) + extraDigits;
        decimals = std::clamp(decimals, 6, 36);

        std::ostringstream stream;
        stream << std::fixed << std::setprecision(decimals) << value;
        std::string text = stream.str();

        while (!text.empty() && text.back() == '0')
            text.pop_back();

        if (!text.empty() && text.back() == '.')
            text.pop_back();

        return text;
    }

    std::string FormatPower10(double exponent, int decimals = 3)
    {
        if (!std::isfinite(exponent))
            return "n/a";

        std::ostringstream stream;
        stream << "10^" << std::fixed << std::setprecision(decimals) << exponent;
        std::string text = stream.str();

        while (!text.empty() && text.back() == '0')
            text.pop_back();

        if (!text.empty() && text.back() == '.')
            text.pop_back();

        return text;
    }
}

bool HUD::IsOpen() const
{
    return open || bodyCreatorOpen;
}

bool HUD::IsAberrationEnabled() const
{
    return aberrationEnabled;
}

int HUD::GetSelectedTargetIndex(const std::vector<CelestialBody>& bodies) const
{
    if (bodies.empty())
        return -1;

    return std::clamp(selectedTarget, 0, static_cast<int>(bodies.size()) - 1);
}

void HUD::UpdateToggle(GLFWwindow* window, double& lastX, double& lastY)
{
    bool wasAnyOpen = open || bodyCreatorOpen;
    bool tabPressed = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
    bool cPressed = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;

    if (tabPressed && !tabWasPressed)
        open = !open;

    if (cPressed && !cWasPressed)
        bodyCreatorOpen = !bodyCreatorOpen;

    bool isAnyOpen = open || bodyCreatorOpen;

    if (isAnyOpen != wasAnyOpen)
    {
        if (isAnyOpen)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwGetCursorPos(window, &lastX, &lastY);
        }
    }

    tabWasPressed = tabPressed;
    cWasPressed = cPressed;
}

void HUD::UpdateNavigation(Camera& camera, const std::vector<CelestialBody>& bodies)
{
    if (!autoSpeed)
        return;

    int targetIndex = GetSelectedTargetIndex(bodies);

    if (targetIndex < 0)
        return;

    const CelestialBody& target = bodies[targetIndex];
    double distanceToCenter = glm::length(target.Position - camera.Position);
    double distanceToSurface = std::max(0.0, distanceToCenter - target.Radius);
    double minSpeed = 10.0;
    double maxSpeed = PhysicsConstants::C * 1.0e12;

    camera.Speed = std::clamp(distanceToSurface * 0.1, minSpeed, maxSpeed);
}

std::string HUD::FormatNumber(double value, int decimals) const
{
    if (!std::isfinite(value))
        return "n/a";

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << value;
    std::string text = stream.str();

    if (text.find('.') != std::string::npos)
    {
        while (!text.empty() && text.back() == '0')
            text.pop_back();

        if (!text.empty() && text.back() == '.')
            text.pop_back();
    }

    std::size_t decimalPosition = text.find('.');
    std::size_t integerEnd = decimalPosition == std::string::npos ? text.size() : decimalPosition;
    std::size_t integerStart = !text.empty() && text[0] == '-' ? 1 : 0;

    for (std::size_t position = integerEnd; position > integerStart + 3;)
    {
        position -= 3;
        text.insert(position, ",");
    }

    return text;
}

std::string HUD::FormatScale(double scale) const
{
    scale = std::max(0.0, scale);

    if (scale >= 1.0)
        return FormatNumber(scale, 3) + "x";

    if (scale >= 0.001)
        return FormatNumber(scale, 6) + "x";

    if (scale >= 0.000001)
        return FormatNumber(scale, 9) + "x";

    if (scale >= 0.000000001)
        return FormatNumber(scale, 12) + "x";

    return FormatNumber(scale, 18) + "x";
}

std::string HUD::FormatDistance(double meters) const
{
    double absoluteMeters = std::abs(meters);
    std::string sign = meters < 0.0 ? "-" : "";

    if (absoluteMeters >= GalacticConstants::Kiloparsec)
        return sign + FormatNumber(absoluteMeters / GalacticConstants::Kiloparsec, 3) + " kpc";

    if (absoluteMeters >= GalacticConstants::Parsec)
        return sign + FormatNumber(absoluteMeters / GalacticConstants::Parsec, 3) + " pc";

    if (absoluteMeters >= GalacticConstants::LightYear * 0.01)
        return sign + FormatNumber(absoluteMeters / GalacticConstants::LightYear, 3) + " ly";

    if (absoluteMeters >= PhysicsConstants::AU * 0.01)
        return sign + FormatNumber(absoluteMeters / PhysicsConstants::AU, 3) + " AU";

    if (absoluteMeters >= 1000000.0)
        return sign + FormatNumber(absoluteMeters / 1000.0, 0) + " km";

    if (absoluteMeters >= 1000.0)
        return sign + FormatNumber(absoluteMeters / 1000.0, 1) + " km";

    return sign + FormatNumber(absoluteMeters, 1) + " m";
}

std::string HUD::FormatTime(double seconds) const
{
    if (seconds < 0.0)
        seconds = 0.0;

    double years = seconds / PhysicsConstants::JulianYear;

    if (!std::isfinite(years))
        return "n/a";

    if (years >= 1.0e15)
        return "~" + FormatPower10(std::log10(years), 4) + " y";

    if (years >= 1000000000.0)
        return FormatNumber(years / 1000000000.0, 3) + " Gyr";

    if (years >= 1000000.0)
        return FormatNumber(years / 1000000.0, 3) + " Myr";

    if (years >= 1000.0)
        return FormatNumber(years / 1000.0, 3) + " kyr";

    if (years >= 1.0)
        return FormatNumber(years, 3) + " y";

    unsigned long long totalSeconds = static_cast<unsigned long long>(seconds);
    unsigned long long days = totalSeconds / 86400ULL;
    unsigned long long hours = (totalSeconds % 86400ULL) / 3600ULL;
    unsigned long long minutes = (totalSeconds % 3600ULL) / 60ULL;
    unsigned long long secs = totalSeconds % 60ULL;

    std::ostringstream stream;

    if (days > 0)
        stream << FormatNumber(static_cast<double>(days), 0) << " d ";

    stream << std::setfill('0') << std::setw(2) << hours << ":" << std::setw(2) << minutes << ":" << std::setw(2) << secs;
    return stream.str();
}

std::string HUD::FormatMass(double mass) const
{
    double solarMasses = mass / PhysicsConstants::SolarMass;
    std::string value;

    if (solarMasses >= 1000.0)
        value = FormatNumber(solarMasses, 0);
    else if (solarMasses >= 1.0)
        value = FormatNumber(solarMasses, 3);
    else if (solarMasses >= 0.001)
        value = FormatNumber(solarMasses, 6);
    else
        value = FormatNumber(solarMasses, 6);

    return value + " M" "\xE2\x98\x89";
}

std::string HUD::FormatRelativisticSpeed(double beta, double log10OneMinusBeta) const
{
    std::ostringstream stream;

    if (std::isfinite(log10OneMinusBeta) && log10OneMinusBeta < -12.0)
    {
        stream << "~1 c  (c - v ~ " << FormatPower10(log10OneMinusBeta, 4) << " c)";
        return stream.str();
    }

    if (beta >= 0.999999999999)
        stream << std::fixed << std::setprecision(15) << beta << " c";
    else
        stream << std::fixed << std::setprecision(12) << beta << " c";

    return stream.str();
}

bool HUD::WorldToScreen(const CelestialBody& body, const Camera& camera, const glm::mat4& view, const glm::mat4& projection, int width, int height, glm::vec2& screen) const
{
    glm::dvec3 relativePosition = body.Position - camera.Position;
    glm::vec3 renderPosition = glm::vec3(relativePosition * RenderConstants::WorldToRenderScale);
    glm::vec4 clip = projection * view * glm::vec4(renderPosition, 1.0f);

    if (clip.w <= 0.0f)
        return false;

    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f || ndc.z < -1.0f || ndc.z > 1.0f)
        return false;

    screen.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(width);
    screen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(height);
    return true;
}

void HUD::DrawNameTags(const Camera& camera, const std::vector<CelestialBody>& bodies, const glm::mat4& view, const glm::mat4& projection, int width, int height) const
{
    if (!showNameTags)
        return;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    for (const CelestialBody& body : bodies)
    {
        if (!body.ShowNameTag)
            continue;

        glm::vec2 screen;

        if (!WorldToScreen(body, camera, view, projection, width, height, screen))
            continue;

        double distance = glm::length(body.Position - camera.Position);
        std::string text = body.Name + "  " + FormatMass(body.Mass) + "  " + FormatDistance(distance);
        ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        ImVec2 position(screen.x - textSize.x * 0.5f, screen.y - 24.0f);
        ImU32 shadowColor = IM_COL32(0, 0, 0, 220);

        drawList->AddText(ImVec2(position.x + 1.0f, position.y + 1.0f), shadowColor, text.c_str());
        drawList->AddText(position, IM_COL32(255, 255, 255, 255), text.c_str());
    }
}

void HUD::DrawGalacticCenterTag(const Camera& camera, const glm::mat4& view, const glm::mat4& projection, int width, int height) const
{
    if (!showNameTags || width <= 0 || height <= 0)
        return;

    glm::dvec3 cameraGalacticParsecs = GalacticFrame::LocalMetersToGalactocentricParsecs(camera.Position);
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImU32 shadowColor = IM_COL32(0, 0, 0, 220);

    auto projectToScreen = [&](const glm::dvec3& worldParsecs, glm::vec2& screen) -> bool
    {
        glm::vec3 relativeParsecs = glm::vec3(worldParsecs - cameraGalacticParsecs);
        glm::vec4 clip = projection * view * glm::vec4(relativeParsecs, 1.0f);

        if (clip.w <= 0.0f)
            return false;

        glm::vec3 ndc = glm::vec3(clip) / clip.w;

        if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f)
            return false;

        screen.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(width);
        screen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(height);
        return true;
    };

    glm::vec2 milkyWayScreen;
    if (projectToScreen(glm::dvec3(0.0), milkyWayScreen))
    {
        double distanceMeters = glm::length(cameraGalacticParsecs) * GalacticConstants::Parsec;
        double massKg = GalacticConstants::SagittariusASolarMasses * PhysicsConstants::SolarMass;
        std::string title = "MILKY WAY GALAXY CENTER";
        std::string subtitle = "Sagittarius A*  " + FormatMass(massKg) + "  " + FormatDistance(distanceMeters);

        ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
        ImVec2 subtitleSize = ImGui::CalcTextSize(subtitle.c_str());
        ImVec2 titlePos(milkyWayScreen.x - titleSize.x * 0.5f, milkyWayScreen.y - 46.0f);
        ImVec2 subtitlePos(milkyWayScreen.x - subtitleSize.x * 0.5f, milkyWayScreen.y - 28.0f);

        drawList->AddText(ImVec2(titlePos.x + 1.0f, titlePos.y + 1.0f), shadowColor, title.c_str());
        drawList->AddText(titlePos, IM_COL32(255, 120, 220, 255), title.c_str());
        drawList->AddText(ImVec2(subtitlePos.x + 1.0f, subtitlePos.y + 1.0f), shadowColor, subtitle.c_str());
        drawList->AddText(subtitlePos, IM_COL32(255, 190, 105, 255), subtitle.c_str());
    }

    glm::vec2 andromedaScreen;
    if (projectToScreen(AndromedaModel::CenterParsecs(), andromedaScreen))
    {
        double distanceMeters = glm::length(AndromedaModel::CenterParsecs() - cameraGalacticParsecs) * GalacticConstants::Parsec;
        std::string title = "ANDROMEDA GALAXY CENTER";
        std::string subtitle = FormatDistance(distanceMeters);

        ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
        ImVec2 subtitleSize = ImGui::CalcTextSize(subtitle.c_str());
        ImVec2 titlePos(andromedaScreen.x - titleSize.x * 0.5f, andromedaScreen.y - 42.0f);
        ImVec2 subtitlePos(andromedaScreen.x - subtitleSize.x * 0.5f, andromedaScreen.y - 24.0f);

        drawList->AddText(ImVec2(titlePos.x + 1.0f, titlePos.y + 1.0f), shadowColor, title.c_str());
        drawList->AddText(titlePos, IM_COL32(255, 222, 92, 255), title.c_str());
        drawList->AddText(ImVec2(subtitlePos.x + 1.0f, subtitlePos.y + 1.0f), shadowColor, subtitle.c_str());
        drawList->AddText(subtitlePos, IM_COL32(255, 235, 150, 235), subtitle.c_str());
    }
}

void HUD::DrawNamedStarTag(const Camera& camera, const NamedStarField& namedStarField, const glm::mat4& view, const glm::mat4& projection, int width, int height) const
{
    const std::vector<NamedStar>& stars = namedStarField.GetStars();

    if (!showNameTags || stars.empty() || width <= 0 || height <= 0)
        return;

    int index = std::clamp(selectedNamedStar, 0, static_cast<int>(stars.size()) - 1);
    const NamedStar& star = stars[index];
    glm::dvec3 cameraParsecs = camera.Position / GalacticConstants::Parsec;
    glm::vec3 relativeParsecs = glm::vec3(star.PositionParsecs - cameraParsecs);
    glm::vec4 clip = projection * view * glm::vec4(relativeParsecs, 1.0f);

    if (clip.w <= 0.0f)
        return;

    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f || ndc.z < -1.0f || ndc.z > 1.0f)
        return;

    float screenX = (ndc.x * 0.5f + 0.5f) * static_cast<float>(width);
    float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * static_cast<float>(height);
    double distanceMeters = glm::length(star.PositionParsecs - cameraParsecs) * GalacticConstants::Parsec;
    std::string text = star.Name + "  " + FormatDistance(distanceMeters);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    ImVec2 position(screenX - textSize.x * 0.5f, screenY - 28.0f);
    ImU32 shadowColor = IM_COL32(0, 0, 0, 220);
    ImU32 textColor = IM_COL32(255, 220, 140, 255);

    drawList->AddText(ImVec2(position.x + 1.0f, position.y + 1.0f), shadowColor, text.c_str());
    drawList->AddText(position, textColor, text.c_str());
}

void HUD::DrawNamedStarSelector(Camera& camera, const NamedStarField& namedStarField)
{
    const std::vector<NamedStar>& stars = namedStarField.GetStars();

    if (stars.empty())
        return;

    selectedNamedStar = std::clamp(selectedNamedStar, 0, static_cast<int>(stars.size()) - 1);
    const char* preview = stars[selectedNamedStar].Name.c_str();

    if (ImGui::BeginCombo("Landmark star", preview))
    {
        for (int i = 0; i < static_cast<int>(stars.size()); i++)
        {
            bool selected = selectedNamedStar == i;

            if (ImGui::Selectable(stars[i].Name.c_str(), selected))
                selectedNamedStar = i;

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    const NamedStar& star = stars[selectedNamedStar];
    glm::dvec3 cameraParsecs = camera.Position / GalacticConstants::Parsec;
    double currentDistanceMeters = glm::length(star.PositionParsecs - cameraParsecs) * GalacticConstants::Parsec;

    ImGui::Text("Spectral type: %s", star.SpectralType.c_str());
    ImGui::Text("Distance from Sun: %s", FormatDistance(star.DistanceParsec * GalacticConstants::Parsec).c_str());
    ImGui::Text("Current distance: %s", FormatDistance(currentDistanceMeters).c_str());

    if (ImGui::Button("Look at landmark star"))
        camera.LookAt(namedStarField.GetLocalPositionMeters(static_cast<std::size_t>(selectedNamedStar)));
}

void HUD::DrawObserverOverlay(const PhysicalObserver& observer, const SimulationClock& simulationClock, const ObserverGravitySystem& observerGravitySystem, const std::vector<CelestialBody>& bodies) const
{
    if (!observer.IsActive())
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 position(viewport->WorkPos.x + viewport->WorkSize.x - 12.0f, viewport->WorkPos.y + viewport->WorkSize.y - 12.0f);

    ImGui::SetNextWindowPos(position, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.45f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;

    ImGui::Begin("Realistic Mode Overlay", nullptr, flags);

    std::string actualSpeedText = FormatRelativisticSpeed(observer.GetBeta(), observer.GetLog10OneMinusBeta());
    std::string selectedSpeedText = FormatRelativisticSpeed(observer.GetSelectedBeta(), observer.GetSelectedLog10OneMinusBeta());

    ImGui::Text("State          %s", observer.IsMotionActive() ? "MOVING" : "HELD");
    ImGui::Text("Actual speed   %s", actualSpeedText.c_str());
    ImGui::Text("Selected speed %s", selectedSpeedText.c_str());
    ImGui::Text("1 - beta       ~%s", FormatPower10(observer.GetLog10OneMinusBeta(), 4).c_str());
    ImGui::Text("Actual gamma   ~%s", FormatPower10(observer.GetLog10Gamma(), 4).c_str());
    ImGui::Text("Selected gamma ~%s", FormatPower10(observer.GetSelectedLog10Gamma(), 4).c_str());
    ImGui::Text("Rapidity       %s", FormatNumber(observer.GetRapidity(), 6).c_str());
    ImGui::Text("Observer time  %s", FormatTime(observer.GetProperTime()).c_str());
    ImGui::Text("World time     %s", FormatTime(simulationClock.GetCoordinateTime()).c_str());
    ImGui::Text("Observer clock 1.000x real");
    ImGui::Text("Base world scale %s", FormatScale(simulationClock.GetTimeScale()).c_str());
    double observerWorldRateLog10 = observer.GetLog10Gamma() + std::log10(std::max(simulationClock.GetTimeScale(), 1.0e-300));
    ImGui::Text("World rate     ~%s x real", FormatPower10(observerWorldRateLog10, 4).c_str());
    ImGui::Text("Traversal u/c ~%s", FormatPower10(observer.GetLog10ProperVelocityC(), 4).c_str());

    const ObserverGravitySample& gravity = observerGravitySystem.GetLastSample();
    double gravityMagnitude = glm::length(gravity.Acceleration);
    ImGui::Text("Gravity        %s g", FormatNumber(gravityMagnitude / PhysicsConstants::StandardGravity, 3).c_str());

    if (gravity.DominantBodyIndex >= 0 && gravity.DominantBodyIndex < static_cast<int>(bodies.size()))
        ImGui::Text("Gravity source %s", bodies[gravity.DominantBodyIndex].Name.c_str());

    if (gravity.DominantIsBlackHole && gravity.DominantSchwarzschildRadius > 0.0)
    {
        double horizonDistance = gravity.DominantDistance - gravity.DominantSchwarzschildRadius;
        ImGui::Text("r / r_s        %s", FormatNumber(gravity.DominantDistance / gravity.DominantSchwarzschildRadius, 4).c_str());

        if (horizonDistance >= 0.0)
            ImGui::Text("Horizon gap    %s", FormatDistance(horizonDistance).c_str());
        else
            ImGui::Text("Inside horizon %s", FormatDistance(-horizonDistance).c_str());
    }

    ImGui::End();
}

void HUD::DrawRocketOverlay(const SpaceRocket& rocket, const SimulationClock& simulationClock) const
{
    if (!rocket.IsActive())
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 position(viewport->WorkPos.x + viewport->WorkSize.x - 12.0f, viewport->WorkPos.y + viewport->WorkSize.y - 12.0f);

    ImGui::SetNextWindowPos(position, ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    ImGui::SetNextWindowBgAlpha(0.45f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs;

    ImGui::Begin("Space Rocket Overlay", nullptr, flags);
    ImGui::Text("State          %s", rocket.IsThrusting() ? "THRUSTING" : "COASTING");
    ImGui::Text("Speed          %s", FormatRelativisticSpeed(rocket.GetBeta(), rocket.GetLog10OneMinusBeta()).c_str());
    ImGui::Text("1 - beta       ~%s", FormatPower10(rocket.GetLog10OneMinusBeta(), 4).c_str());
    ImGui::Text("c gap          ~%s m/s", FormatPower10(rocket.GetLog10SpeedGapMetersPerSecond(), 4).c_str());
    ImGui::Text("Gamma          ~%s", FormatPower10(rocket.GetLog10Gamma(), 4).c_str());
    ImGui::Text("Rapidity       %s", FormatNumber(rocket.GetRapidity(), 9).c_str());
    ImGui::Text("Thrust         %s N", FormatNumber(rocket.GetEngineThrust(), 0).c_str());
    ImGui::Text("Engine accel   %s g", FormatNumber(rocket.GetRestAccelerationG(), 3).c_str());
    ImGui::Text("Actual accel   %s g", FormatNumber(rocket.GetCoordinateAccelerationG(), 3).c_str());
    ImGui::Text("Momentum       ~%s kg m/s", FormatPower10(rocket.GetLog10MomentumMagnitude(), 4).c_str());
    ImGui::Text("Rocket time    %s", FormatTime(rocket.GetProperTime()).c_str());
    ImGui::Text("Rocket clock   1.000x real");
    ImGui::Text("World time     %s", FormatTime(simulationClock.GetCoordinateTime()).c_str());
    double rocketWorldRateLog10 = rocket.GetLog10Gamma() + std::log10(std::max(simulationClock.GetTimeScale(), 1.0e-300));
    ImGui::Text("World rate     ~%s x real", FormatPower10(rocketWorldRateLog10, 4).c_str());
    ImGui::End();
}

void HUD::DrawCrosshair(int width, int height) const
{
    if (width <= 0 || height <= 0)
        return;

    float x = static_cast<float>(width) * 0.5f;
    float y = static_cast<float>(height) * 0.5f;
    float gap = 3.0f;
    float arm = 7.0f;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImU32 shadow = IM_COL32(0, 0, 0, 210);
    ImU32 color = IM_COL32(225, 232, 240, 205);

    drawList->AddLine(ImVec2(x - arm - 1.0f, y + 1.0f), ImVec2(x - gap - 1.0f, y + 1.0f), shadow, 2.0f);
    drawList->AddLine(ImVec2(x + gap + 1.0f, y + 1.0f), ImVec2(x + arm + 1.0f, y + 1.0f), shadow, 2.0f);
    drawList->AddLine(ImVec2(x + 1.0f, y - arm - 1.0f), ImVec2(x + 1.0f, y - gap - 1.0f), shadow, 2.0f);
    drawList->AddLine(ImVec2(x + 1.0f, y + gap + 1.0f), ImVec2(x + 1.0f, y + arm + 1.0f), shadow, 2.0f);

    drawList->AddLine(ImVec2(x - arm, y), ImVec2(x - gap, y), color, 1.0f);
    drawList->AddLine(ImVec2(x + gap, y), ImVec2(x + arm, y), color, 1.0f);
    drawList->AddLine(ImVec2(x, y - arm), ImVec2(x, y - gap), color, 1.0f);
    drawList->AddLine(ImVec2(x, y + gap), ImVec2(x, y + arm), color, 1.0f);
}

void HUD::DrawTargetSelector(const Camera& camera, const std::vector<CelestialBody>& bodies)
{
    int targetIndex = GetSelectedTargetIndex(bodies);

    if (targetIndex < 0)
        return;

    if (selectedTarget != targetIndex)
        selectedTarget = targetIndex;

    const char* preview = bodies[selectedTarget].Name.c_str();

    if (ImGui::BeginCombo("Target", preview))
    {
        for (int i = 0; i < static_cast<int>(bodies.size()); i++)
        {
            bool selected = selectedTarget == i;

            if (ImGui::Selectable(bodies[i].Name.c_str(), selected))
                selectedTarget = i;

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    const CelestialBody& target = bodies[selectedTarget];
    double centerDistance = glm::length(target.Position - camera.Position);
    double surfaceDistance = std::max(0.0, centerDistance - target.Radius);

    ImGui::Text("Mass: %s", FormatMass(target.Mass).c_str());

    if (target.Type == CelestialBodyType::BlackHole)
    {
        double schwarzschildRadius = PhysicsConstants::SchwarzschildRadius(target.Mass);
        double horizonDistance = centerDistance - schwarzschildRadius;

        ImGui::Text("Center distance: %s", FormatDistance(centerDistance).c_str());
        ImGui::Text("Event horizon: %s", FormatDistance(schwarzschildRadius).c_str());

        if (horizonDistance >= 0.0)
            ImGui::Text("Distance to horizon: %s", FormatDistance(horizonDistance).c_str());
        else
            ImGui::Text("Inside horizon by: %s", FormatDistance(-horizonDistance).c_str());

        ImGui::Text("r / r_s: %s", FormatNumber(centerDistance / schwarzschildRadius, 4).c_str());
        ImGui::TextDisabled("Black-hole gravity is still a Newtonian stress test. GR comes next.");
    }
    else
    {
        ImGui::Text("Center distance: %s", FormatDistance(centerDistance).c_str());
        ImGui::Text("Surface distance: %s", FormatDistance(surfaceDistance).c_str());
    }
}

void HUD::DrawBodyCreator(Camera& camera, std::vector<CelestialBody>& bodies, int width, int height)
{
    if (!bodyCreatorOpen)
        return;

    ImGui::SetNextWindowSize(ImVec2(430.0f, 560.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Celestial Body Creator");

    const char* typeNames[] = { "Planet", "Star", "Neutron Star", "Black Hole" };
    int oldType = spawnType;

    if (ImGui::Combo("Type", &spawnType, typeNames, 4) && spawnType != oldType)
    {
        if (spawnType == 0)
        {
            spawnMassSolarMasses = PhysicsConstants::EarthMass / PhysicsConstants::SolarMass;
            spawnRadiusKm = PhysicsConstants::EarthRadius / 1000.0;
        }
        else if (spawnType == 1)
        {
            spawnMassSolarMasses = 1.0;
            spawnRadiusKm = PhysicsConstants::SolarRadius / 1000.0;
        }
        else if (spawnType == 2)
        {
            spawnMassSolarMasses = 1.4;
            spawnRadiusKm = 12.0;
        }
        else
        {
            spawnMassSolarMasses = 10.0;
            spawnRadiusKm = PhysicsConstants::SchwarzschildRadius(spawnMassSolarMasses * PhysicsConstants::SolarMass) / 1000.0;
        }
    }

    ImGui::InputText("Name", spawnName, sizeof(spawnName));
    ImGui::InputDouble("Mass (M_sun)", &spawnMassSolarMasses, 0.0, 0.0, "%.12g");

    spawnMassSolarMasses = std::max(spawnMassSolarMasses, 1.0e-18);
    double massKg = spawnMassSolarMasses * PhysicsConstants::SolarMass;

    if (spawnType == 3)
    {
        spawnRadiusKm = PhysicsConstants::SchwarzschildRadius(massKg) / 1000.0;
        ImGui::Text("Schwarzschild radius: %s", FormatDistance(spawnRadiusKm * 1000.0).c_str());
    }
    else
    {
        ImGui::InputDouble("Radius (km)", &spawnRadiusKm, 0.0, 0.0, "%.9g");
        spawnRadiusKm = std::max(spawnRadiusKm, 0.001);
    }

    ImGui::Text("Mass: %s", FormatMass(massKg).c_str());

    ImGui::Separator();
    ImGui::TextUnformatted("Placement");

    double minDistance = 1000.0;
    double maxDistance = GalacticConstants::Kiloparsec * 50.0;
    spawnDistanceMeters = std::clamp(spawnDistanceMeters, minDistance, maxDistance);

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::SliderScalar("Range", ImGuiDataType_Double, &spawnDistanceMeters, &minDistance, &maxDistance, " ", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoInput);
    ImGui::Text("Crosshair range: %s", FormatDistance(spawnDistanceMeters).c_str());
    ImGui::TextDisabled("The new body is placed on the camera-forward ray.");

    ImGui::Separator();
    ImGui::TextUnformatted("Initial velocity");

    int targetIndex = GetSelectedTargetIndex(bodies);
    const CelestialBody* target = targetIndex >= 0 ? &bodies[targetIndex] : nullptr;

    ImGui::Checkbox("Initialize circular binary with selected target", &spawnCircularBinary);

    if (spawnCircularBinary)
    {
        if (target)
            ImGui::Text("Binary target: %s", target->Name.c_str());
        else
            ImGui::TextDisabled("No target is available.");
    }
    else
    {
        ImGui::Checkbox("Inherit selected target velocity", &spawnInheritTargetVelocity);
        ImGui::InputDouble("Forward (km/s)", &spawnForwardKmS, 0.0, 0.0, "%.6g");
        ImGui::InputDouble("Right (km/s)", &spawnRightKmS, 0.0, 0.0, "%.6g");
        ImGui::InputDouble("Up (km/s)", &spawnUpKmS, 0.0, 0.0, "%.6g");
    }

    ImGui::Checkbox("Show name tag", &spawnShowNameTag);

    glm::dvec3 placementPosition = camera.Position + camera.Front() * spawnDistanceMeters;

    if (ImGui::Button("Place at crosshair", ImVec2(-1.0f, 34.0f)))
    {
        CelestialBodyType type = CelestialBodyType::Planet;

        if (spawnType == 1)
            type = CelestialBodyType::Star;
        else if (spawnType == 2)
            type = CelestialBodyType::NeutronStar;
        else if (spawnType == 3)
            type = CelestialBodyType::BlackHole;

        double radiusMeters = spawnRadiusKm * 1000.0;
        glm::dvec3 velocity =
            camera.Front() * (spawnForwardKmS * 1000.0)
            + camera.Right() * (spawnRightKmS * 1000.0)
            + glm::dvec3(0.0, 1.0, 0.0) * (spawnUpKmS * 1000.0);

        if (!spawnCircularBinary && spawnInheritTargetVelocity && target)
            velocity += target->Velocity;

        if (spawnCircularBinary && target && target->Mass > 0.0)
        {
            glm::dvec3 radial = placementPosition - target->Position;
            double separation = glm::length(radial);

            if (separation > 0.0)
            {
                glm::dvec3 radialDirection = radial / separation;
                glm::dvec3 tangent = glm::cross(glm::dvec3(0.0, 1.0, 0.0), radialDirection);

                if (glm::length(tangent) < 1.0e-9)
                    tangent = glm::cross(camera.Right(), radialDirection);

                tangent = glm::normalize(tangent);

                double totalMass = target->Mass + massKg;
                double relativeSpeed = std::sqrt(PhysicsConstants::G * totalMass / separation);
                glm::dvec3 centerVelocity = target->Velocity;

                bodies[targetIndex].Velocity = centerVelocity - tangent * relativeSpeed * (massKg / totalMass);
                velocity = centerVelocity + tangent * relativeSpeed * (target->Mass / totalMass);
            }
        }

        std::string name = spawnName;

        if (name.empty())
            name = "Body " + std::to_string(bodies.size() + 1);

        CelestialBody body(name, type, massKg, radiusMeters, placementPosition, velocity);
        body.ShowNameTag = spawnShowNameTag;
        bodies.push_back(body);
    }

    ImGui::Separator();
    ImGui::Text("Bodies in simulation: %d", static_cast<int>(bodies.size()));
    ImGui::TextDisabled("C closes this window. Tab opens the main simulation HUD.");

    ImGui::End();

    if (width > 0 && height > 0)
    {
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        float x = static_cast<float>(width) * 0.5f;
        float y = static_cast<float>(height) * 0.5f;
        std::string rangeText = FormatDistance(spawnDistanceMeters);
        ImVec2 textSize = ImGui::CalcTextSize(rangeText.c_str());
        ImVec2 pos(x - textSize.x * 0.5f, y + 16.0f);
        drawList->AddText(ImVec2(pos.x + 1.0f, pos.y + 1.0f), IM_COL32(0, 0, 0, 220), rangeText.c_str());
        drawList->AddText(pos, IM_COL32(220, 232, 245, 230), rangeText.c_str());
    }
}

void HUD::Draw(Camera& camera, std::vector<CelestialBody>& bodies, const NamedStarField& namedStarField, SimulationClock& simulationClock, PhysicalObserver& observer, SpaceRocket& rocket, const ObserverGravitySystem& observerGravitySystem, const glm::mat4& view, const glm::mat4& projection, int width, int height)
{
    DrawNameTags(camera, bodies, view, projection, width, height);
    DrawGalacticCenterTag(camera, view, projection, width, height);
    DrawNamedStarTag(camera, namedStarField, view, projection, width, height);
    DrawObserverOverlay(observer, simulationClock, observerGravitySystem, bodies);
    DrawRocketOverlay(rocket, simulationClock);
    DrawCrosshair(width, height);
    DrawBodyCreator(camera, bodies, width, height);

    if (!open)
        return;

    ImGui::SetNextWindowSize(ImVec2(470.0f, 590.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation HUD");

    ImGui::TextUnformatted("Time");
    ImGui::Text("Coordinate time: %s", FormatTime(simulationClock.GetCoordinateTime()).c_str());

    if (observer.IsActive())
        ImGui::Text("Observer proper time: %s", FormatTime(observer.GetProperTime()).c_str());
    else if (rocket.IsActive())
        ImGui::Text("Rocket proper time: %s", FormatTime(rocket.GetProperTime()).c_str());

    if (ImGui::Button(simulationClock.IsPaused() ? "Resume" : "Pause"))
        simulationClock.TogglePause();

    double currentTimeScale = std::max(simulationClock.GetTimeScale(), 1.0e-18);
    double timeScaleExponent = std::log10(currentTimeScale);
    double minTimeScaleExponent = -18.0;
    double maxTimeScaleExponent = std::log10(PhysicsConstants::JulianYear * 1000.0);

    ImGui::SetNextItemWidth(-1.0f);

    if (ImGui::SliderScalar("Time scale", ImGuiDataType_Double, &timeScaleExponent, &minTimeScaleExponent, &maxTimeScaleExponent, " ", ImGuiSliderFlags_NoInput))
        simulationClock.SetTimeScale(std::pow(10.0, timeScaleExponent));

    ImGui::Text("Playback: %s", FormatScale(simulationClock.GetTimeScale()).c_str());

    if (ImGui::BeginTable("TimeScalePresets", 4, ImGuiTableFlags_SizingStretchSame))
    {
        const char* labels[] = { "Minimum", "Very slow", "0.001x", "0.1x", "1x", "60x", "1 hour/s", "1 day/s", "1 year/s", "10 years/s", "100 years/s", "1000 years/s" };
        const double values[] = { 1.0e-18, 1.0e-6, 0.001, 0.1, 1.0, 60.0, 3600.0, 86400.0, PhysicsConstants::JulianYear, PhysicsConstants::JulianYear * 10.0, PhysicsConstants::JulianYear * 100.0, PhysicsConstants::JulianYear * 1000.0 };

        for (int i = 0; i < 12; i++)
        {
            ImGui::TableNextColumn();

            if (ImGui::Button(labels[i], ImVec2(-1.0f, 0.0f)))
                simulationClock.SetTimeScale(values[i]);
        }

        ImGui::EndTable();
    }

    if (observer.IsActive())
    {
        ImGui::TextUnformatted("Observer clock: 1.000x real time");
        ImGui::Text("World playback: %s", FormatScale(simulationClock.GetTimeScale()).c_str());
        ImGui::Text("Physical gamma: ~%s", FormatPower10(observer.GetLog10Gamma(), 4).c_str());
        ImGui::Text("Traversal u/c: ~%s", FormatPower10(observer.GetLog10ProperVelocityC(), 4).c_str());
        ImGui::TextDisabled("Observer clock stays at 1x. World coordinate time advances by gamma times the selected base playback scale.");
    }
    else if (rocket.IsActive())
    {
        ImGui::TextUnformatted("Rocket clock: 1.000x real time");
        ImGui::Text("World playback: %s", FormatScale(simulationClock.GetTimeScale()).c_str());
        ImGui::Text("Physical gamma: ~%s", FormatPower10(rocket.GetLog10Gamma(), 4).c_str());
        ImGui::Text("Traversal u/c: ~%s", FormatPower10(rocket.GetLog10ProperVelocityC(), 4).c_str());
        ImGui::TextDisabled("Rocket clock stays at 1x. World coordinate time advances by gamma times the selected base playback scale.");
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Target");
    DrawTargetSelector(camera, bodies);

    ImGui::Separator();
    ImGui::TextUnformatted("Galactic frame");

    glm::dvec3 cameraGalacticParsecs = GalacticFrame::LocalMetersToGalactocentricParsecs(camera.Position);
    double galacticCenterDistance = glm::length(cameraGalacticParsecs) * GalacticConstants::Parsec;

    ImGui::Text("Galactic X: %s pc", FormatNumber(cameraGalacticParsecs.x, 3).c_str());
    ImGui::Text("Galactic Y: %s pc", FormatNumber(cameraGalacticParsecs.y, 3).c_str());
    ImGui::Text("Galactic Z: %s pc", FormatNumber(cameraGalacticParsecs.z, 3).c_str());
    ImGui::Text("Distance to Sgr A*: %s", FormatDistance(galacticCenterDistance).c_str());

    if (!observer.IsActive() && !rocket.IsActive())
    {
        if (ImGui::Button("Look at Sagittarius A*"))
            camera.LookAt(GalacticFrame::GalacticCenterLocalMeters());
    }

    ImGui::TextDisabled("Galactic coordinates use Sgr A* as origin. Solar-system physics remains in the local meter frame.");

    ImGui::Separator();
    ImGui::TextUnformatted("Named star landmarks");
    DrawNamedStarSelector(camera, namedStarField);

    ImGui::Separator();

    if (!observer.IsActive() && !rocket.IsActive())
    {
        ImGui::TextUnformatted("Free Navigation");

        if (ImGui::Button("Enter Realistic Mode"))
        {
            simulationClock.SetTimeScale(1.0);
            observer.Enter(camera.Position, simulationClock.GetCoordinateTime());
        }

        ImGui::SameLine();

        if (ImGui::Button("Enter Space Rocket"))
        {
            simulationClock.SetTimeScale(1.0);
            rocket.Enter(camera.Position, simulationClock.GetCoordinateTime());
        }

        int targetIndex = GetSelectedTargetIndex(bodies);

        if (targetIndex >= 0)
        {
            const CelestialBody& target = bodies[targetIndex];

            if (ImGui::Button("Focus"))
                camera.LookAt(target.Position);

            ImGui::SameLine();

            if (ImGui::Button("Approach"))
            {
                glm::dvec3 direction = camera.Position - target.Position;

                if (glm::length(direction) < 1.0)
                    direction = glm::dvec3(0.0, 0.0, 1.0);

                direction = glm::normalize(direction);
                camera.Position = target.Position + direction * (target.Radius * 5.0);
                camera.LookAt(target.Position);
            }
        }

        ImGui::Checkbox("Auto speed", &autoSpeed);

        double minSpeed = 1.0;
        double maxSpeed = PhysicsConstants::C * 1.0e12;

        if (!autoSpeed)
            ImGui::SliderScalar("Navigation speed", ImGuiDataType_Double, &camera.Speed, &minSpeed, &maxSpeed, " ", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoInput);

        ImGui::Text("Navigation speed: %.3f km/s", camera.Speed / 1000.0);
        ImGui::Text("Navigation speed: %s c", FormatNumber(camera.Speed / PhysicsConstants::C, 3).c_str());
    }
    else if (observer.IsActive())
    {
        ImGui::TextUnformatted("Realistic Mode");

        if (ImGui::Button("Return to Free Navigation"))
            observer.Exit();

        ImGui::SameLine();

        if (ImGui::Button("Hold / Zero Velocity"))
            observer.Hold();

        double selectedBeta = observer.GetSelectedBeta();

        ImGui::Text("State: %s", observer.IsMotionActive() ? "MOVING" : "HELD");
        ImGui::Text("Spacecraft mass: %.0f kg", observer.GetMass());
        ImGui::Text("Selected speed: %s", FormatRelativisticSpeed(selectedBeta, observer.GetSelectedLog10OneMinusBeta()).c_str());
        ImGui::Text("Selected 1 - beta: ~%s", FormatPower10(observer.GetSelectedLog10OneMinusBeta(), 4).c_str());
        ImGui::Text("Selected gamma: ~%s", FormatPower10(observer.GetSelectedLog10Gamma(), 4).c_str());
        ImGui::Text("Selected rapidity: %s", FormatNumber(observer.GetSelectedRapidity(), 6).c_str());
        ImGui::Text("Actual speed: %s", FormatRelativisticSpeed(observer.GetBeta(), observer.GetLog10OneMinusBeta()).c_str());

        ImGui::Checkbox("Aberration / relativistic visuals", &aberrationEnabled);
        ImGui::TextDisabled("Toggles aberration, Doppler-style color shift and motion streak visuals in Realistic Mode.");

        if (observer.IsAtNumericalLimit())
            ImGui::TextUnformatted("Stored rapidity reached the floating-point storage limit.");

        ImGui::Text("1 - beta: ~%s", FormatPower10(observer.GetLog10OneMinusBeta(), 4).c_str());
        ImGui::Text("Gamma: ~%s", FormatPower10(observer.GetLog10Gamma(), 4).c_str());
        ImGui::Text("Rapidity: %s", FormatNumber(observer.GetRapidity(), 6).c_str());
        ImGui::Text("Traversal u/c: ~%s", FormatPower10(observer.GetLog10ProperVelocityC(), 4).c_str());
        ImGui::Text("Observer time: %s", FormatTime(observer.GetProperTime()).c_str());

        const ObserverGravitySample& gravity = observerGravitySystem.GetLastSample();
        double gravityMagnitude = glm::length(gravity.Acceleration);

        ImGui::Text("Sampled gravity: %s m/s^2", FormatNumber(gravityMagnitude, 3).c_str());
        ImGui::Text("Sampled gravity: %s g", FormatNumber(gravityMagnitude / PhysicsConstants::StandardGravity, 3).c_str());

        if (gravity.DominantBodyIndex >= 0 && gravity.DominantBodyIndex < static_cast<int>(bodies.size()))
            ImGui::Text("Dominant gravity source: %s", bodies[gravity.DominantBodyIndex].Name.c_str());

        if (gravity.DominantIsBlackHole && gravity.DominantSchwarzschildRadius > 0.0)
        {
            double horizonDistance = gravity.DominantDistance - gravity.DominantSchwarzschildRadius;
            ImGui::Text("Black-hole center: %s", FormatDistance(gravity.DominantDistance).c_str());
            ImGui::Text("Event horizon: %s", FormatDistance(gravity.DominantSchwarzschildRadius).c_str());

            if (horizonDistance >= 0.0)
                ImGui::Text("Distance to horizon: %s", FormatDistance(horizonDistance).c_str());
            else
                ImGui::Text("Inside horizon by: %s", FormatDistance(-horizonDistance).c_str());

            ImGui::Text("r / r_s: %s", FormatNumber(gravity.DominantDistance / gravity.DominantSchwarzschildRadius, 4).c_str());

            if (gravity.DominantBodyIndex >= 0)
            {
                glm::dvec3 toBlackHole = bodies[gravity.DominantBodyIndex].Position - observer.GetPosition();
                double toBlackHoleLength = glm::length(toBlackHole);

                if (!observer.IsMotionActive())
                    ImGui::TextUnformatted("Radial motion: HELD");
                else if (toBlackHoleLength > 0.0)
                {
                    double towardSpeed = glm::dot(observer.GetVelocity(), toBlackHole / toBlackHoleLength);

                    if (std::abs(towardSpeed) < 1.0)
                        ImGui::TextUnformatted("Radial motion: SIDEWAYS");
                    else if (towardSpeed > 0.0)
                        ImGui::TextUnformatted("Radial motion: TOWARD");
                    else
                        ImGui::TextUnformatted("Radial motion: AWAY");
                }
            }

            ImGui::TextDisabled("Strong-field GR is not enabled yet; this is an extreme-gravity test source.");
        }

        ImGui::TextUnformatted("Left Shift: increase selected speed");
        ImGui::TextUnformatted("Left Ctrl: decrease selected speed");
        ImGui::TextUnformatted("W: move at selected speed toward current mouse view");
        ImGui::TextUnformatted("While W is held, steering follows the camera continuously");
        ImGui::TextUnformatted("Release W: immediately hold and zero velocity");
        ImGui::TextUnformatted("Hold button: zero observer velocity");
        ImGui::TextUnformatted("Crosshair marks the camera forward direction");
        ImGui::TextDisabled("Observer/camera gravity is disabled. Newtonian gravity is simulated only between celestial bodies.");
    }
    else
    {
        ImGui::TextUnformatted("Space Rocket Mode");

        if (ImGui::Button("Return to Free Navigation"))
            rocket.Exit();

        ImGui::SameLine();

        if (ImGui::Button("Reset Rocket Velocity"))
            rocket.ResetVelocity();

        double rocketMass = rocket.GetMass();
        double minMass = 1000.0;
        double maxMass = 1000000000.0;

        if (ImGui::SliderScalar("Rocket mass (kg)", ImGuiDataType_Double, &rocketMass, &minMass, &maxMass, " ", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoInput))
            rocket.SetMass(rocketMass);

        double thrust = std::max(rocket.GetEngineThrust(), 1000.0);
        double minThrust = 1000.0;
        double maxThrust = 1.0e16;

        if (ImGui::SliderScalar("Engine thrust (N)", ImGuiDataType_Double, &thrust, &minThrust, &maxThrust, " ", ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_NoInput))
            rocket.SetEngineThrust(thrust);

        ImGui::Text("State: %s", rocket.IsThrusting() ? "THRUSTING" : "COASTING");
        ImGui::Text("Mass: %s kg", FormatNumber(rocket.GetMass(), 0).c_str());
        ImGui::Text("Thrust: %s N", FormatNumber(rocket.GetEngineThrust(), 0).c_str());
        ImGui::Text("Engine acceleration: %s m/s^2", FormatNumber(rocket.GetRestAcceleration(), 3).c_str());
        ImGui::Text("Engine acceleration: %s g", FormatNumber(rocket.GetRestAccelerationG(), 3).c_str());
        ImGui::Text("Actual coordinate acceleration: %s m/s^2", FormatNumber(rocket.GetCoordinateAcceleration(), 3).c_str());
        ImGui::Text("Speed: %s", FormatRelativisticSpeed(rocket.GetBeta(), rocket.GetLog10OneMinusBeta()).c_str());
        ImGui::Text("1 - beta: ~%s", FormatPower10(rocket.GetLog10OneMinusBeta(), 4).c_str());
        ImGui::Text("Speed gap to c: ~%s m/s", FormatPower10(rocket.GetLog10SpeedGapMetersPerSecond(), 4).c_str());
        ImGui::Text("Gamma: ~%s", FormatPower10(rocket.GetLog10Gamma(), 4).c_str());
        ImGui::Text("Rapidity: %s", FormatNumber(rocket.GetRapidity(), 9).c_str());
        ImGui::Text("Momentum |p|: ~%s kg m/s", FormatPower10(rocket.GetLog10MomentumMagnitude(), 4).c_str());
        ImGui::Text("Traversal u/c: ~%s", FormatPower10(rocket.GetLog10ProperVelocityC(), 4).c_str());
        ImGui::Text("Rocket proper time: %s", FormatTime(rocket.GetProperTime()).c_str());

        ImGui::Checkbox("Aberration / relativistic visuals", &aberrationEnabled);

        if (rocket.IsAtNumericalLimit())
            ImGui::TextUnformatted("Log-momentum storage reached a non-finite value.");

        ImGui::TextUnformatted("W / S: forward / reverse thrust");
        ImGui::TextUnformatted("A / D: left / right lateral thrusters");
        ImGui::TextUnformatted("Space / Left Ctrl: up / down thrusters");
        ImGui::TextUnformatted("Mouse changes rocket nose direction, not current momentum");
        ImGui::TextUnformatted("Release all thrust keys: coast with existing momentum");
        ImGui::TextDisabled("No rapidity substep ladder is used. Momentum magnitude is accumulated in logarithmic form.");
        ImGui::TextDisabled("At extreme gamma, speed is shown as ~1 c together with the remaining c - v gap in powers of ten.");
        ImGui::TextDisabled("Rocket time runs at 1x real time. World time scale does not control rocket traversal; coordinate speed remains below c while proper velocity grows with gamma.");
        ImGui::TextDisabled("External observer gravity remains disabled; this mode currently models thrust, inertia and momentum.");
    }

    ImGui::Separator();
    ImGui::Checkbox("Name tags", &showNameTags);

    ImGui::End();
}
