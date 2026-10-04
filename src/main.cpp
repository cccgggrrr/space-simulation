#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "HUD.h"
#include "Shader.h"
#include "Camera.h"
#include "GaiaStarField.h"
#include "MilkyWayDensityRenderer.h"
#include "GalacticCenterMarker.h"
#include "GalacticFrame.h"
#include "NamedStarField.h"
#include "MilkyWayDensityField.h"
#include "LocalMilkyWayStarField.h"
#include "CelestialBody.h"
#include "SphereRenderer.h"
#include "GravitySystem.h"
#include "PhysicsConstants.h"
#include "SimulationClock.h"
#include "PhysicalObserver.h"
#include "SpaceRocket.h"
#include "ObserverGravitySystem.h"
#include "GravitationalLensingRenderer.h"
#include "AndromedaDensityRenderer.h"
#include "AndromedaModel.h"
#include "LocalAndromedaStarField.h"

int main()
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Space-Time Simulation", nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    {
        Shader starShader("shaders/gaia_star.vert", "shaders/gaia_star.frag");
        Shader localStarShader("shaders/local_milkyway_star.vert", "shaders/local_milkyway_star.frag");
        Shader localAndromedaStarShader("shaders/local_andromeda_star.vert", "shaders/local_andromeda_star.frag");
        Shader milkyWayDensityShader("shaders/milkyway_density.vert", "shaders/milkyway_density.frag");
        Shader andromedaDensityShader("shaders/andromeda_density.vert", "shaders/andromeda_density.frag");
        Shader galacticCenterShader("shaders/galactic_center.vert", "shaders/galactic_center.frag");
        Shader namedStarShader("shaders/named_star.vert", "shaders/named_star.frag");
        Shader bodyShader("shaders/body.vert", "shaders/body.frag");

        Camera camera;
        GaiaStarField starField("data/gaia_dr3_10000.csv");
        LocalMilkyWayStarField localMilkyWayStarField;
        LocalAndromedaStarField localAndromedaStarField;
        MilkyWayDensityRenderer milkyWayDensityRenderer(470000);
        AndromedaDensityRenderer andromedaDensityRenderer(260000);
        GalacticCenterMarker galacticCenterMarker;
        NamedStarField namedStarField;
        SphereRenderer sphereRenderer;
        GravitySystem gravitySystem;
        SimulationClock simulationClock(3600.0);
        PhysicalObserver observer;
        SpaceRocket rocket;
        ObserverGravitySystem observerGravitySystem;
        HUD hud;
        GravitationalLensingRenderer gravitationalLensingRenderer;

        std::vector<CelestialBody> bodies;
        camera.Position = glm::dvec3(0.0, 0.0, 3.0e7);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        double lastX;
        double lastY;
        glfwGetCursorPos(window, &lastX, &lastY);

        double lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(window))
        {
            double currentTime = glfwGetTime();
            double deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            if (deltaTime > 0.1)
                deltaTime = 0.1;

            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            hud.UpdateToggle(window, lastX, lastY);

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, GLFW_TRUE);

            if (!hud.IsOpen() && observer.IsActive())
                observer.ProcessSelectedSpeedControl(deltaTime, glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS, glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);

            static bool realisticWWasPressed = false;
            bool realisticWPressed = observer.IsActive() && !hud.IsOpen() && glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;

            if (observer.IsActive())
            {
                if (realisticWPressed && !realisticWWasPressed)
                    observer.Launch(camera.Front());
                else if (realisticWPressed)
                    observer.Steer(camera.Front());
                else if (realisticWWasPressed)
                    observer.Hold();

                realisticWWasPressed = realisticWPressed;
            }
            else
                realisticWWasPressed = false;

            glm::dvec3 rocketThrustDirection(0.0);

            if (rocket.IsActive() && !hud.IsOpen())
            {
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                    rocketThrustDirection += camera.Front();
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                    rocketThrustDirection -= camera.Front();
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                    rocketThrustDirection += camera.Right();
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                    rocketThrustDirection -= camera.Right();
                if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
                    rocketThrustDirection += glm::dvec3(0.0, 1.0, 0.0);
                if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
                    rocketThrustDirection -= glm::dvec3(0.0, 1.0, 0.0);
            }

            double localClockDelta = simulationClock.IsPaused() ? 0.0 : deltaTime;
            double baseWorldDelta = localClockDelta * simulationClock.GetTimeScale();
            double activeLog10GammaForClock = observer.IsActive() ? observer.GetLog10Gamma() : rocket.IsActive() ? rocket.GetLog10Gamma() : 0.0;
            double worldGamma = 1.0;

            if (std::isfinite(activeLog10GammaForClock) && activeLog10GammaForClock > 0.0)
                worldGamma = std::pow(10.0, std::min(activeLog10GammaForClock, 300.0));

            double worldCoordinateDelta = baseWorldDelta * worldGamma;

            if (!std::isfinite(worldCoordinateDelta))
                worldCoordinateDelta = std::numeric_limits<double>::max() * 1.0e-6;

            double simulationDelta = simulationClock.AdvanceCoordinate(worldCoordinateDelta);

            gravitySystem.Update(bodies, baseWorldDelta);

            if (observer.IsActive())
            {
                observer.IntegrateStep(simulationDelta, localClockDelta, glm::dvec3(0.0));
                observerGravitySystem.Update(observer, bodies, 0.0);
                camera.Position = observer.GetPosition();
            }
            else if (rocket.IsActive())
            {
                rocket.Update(localClockDelta, simulationDelta, rocketThrustDirection);
                camera.Position = rocket.GetPosition();
            }
            else
                hud.UpdateNavigation(camera, bodies);

            if (!hud.IsOpen() && !observer.IsActive() && !rocket.IsActive())
                camera.ProcessKeyboard(deltaTime, glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS, glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS, glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS, glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS, glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS, glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);

            double mouseX;
            double mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);

            double xOffset = mouseX - lastX;
            double yOffset = lastY - mouseY;

            lastX = mouseX;
            lastY = mouseY;

            if (!hud.IsOpen())
                camera.ProcessMouse(xOffset, yOffset);

            int width;
            int height;
            glfwGetFramebufferSize(window, &width, &height);

            if (height == 0)
                height = 1;

            gravitationalLensingRenderer.Begin(width, height);

            glViewport(0, 0, width, height);
            glClearColor(0.002f, 0.003f, 0.008f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = camera.GetViewRotationMatrix();
            float aspect = static_cast<float>(width) / static_cast<float>(height);
            glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.001f, 50000.0f);
            glm::mat4 galaxyProjection = glm::perspective(glm::radians(60.0f), aspect, 0.001f, 2000000.0f);
            glm::mat4 starProjection = glm::perspective(glm::radians(60.0f), aspect, 1.0e-8f, 50000.0f);

            glm::dvec3 activeVelocity = observer.IsActive() ? observer.GetVelocity() : rocket.IsActive() ? rocket.GetVelocity() : glm::dvec3(0.0);
            double activeSpeed = glm::length(activeVelocity);
            bool relativisticVisualsEnabled = (observer.IsActive() || rocket.IsActive()) && activeSpeed > 1.0 && hud.IsAberrationEnabled();
            glm::dvec3 observerVelocityDirectionD = relativisticVisualsEnabled ? activeVelocity / activeSpeed : glm::dvec3(0.0, 0.0, -1.0);
            glm::vec3 observerVelocityDirection = glm::vec3(observerVelocityDirectionD);
            glm::vec3 observerVelocityDirectionView = glm::mat3(view) * observerVelocityDirection;
            double activeLog10Gamma = observer.IsActive() ? observer.GetLog10Gamma() : rocket.IsActive() ? rocket.GetLog10Gamma() : 0.0;
            double visualGammaValue = std::pow(10.0, std::clamp(activeLog10Gamma, 0.0, 18.0));
            float relativisticGamma = relativisticVisualsEnabled ? static_cast<float>(visualGammaValue) : 1.0f;
            double activeLog10ProperVelocityC = observer.IsActive() ? observer.GetLog10ProperVelocityC() : rocket.IsActive() ? rocket.GetLog10ProperVelocityC() : -std::numeric_limits<double>::infinity();
            double log10PersistencePc = std::log10(PhysicsConstants::C * 0.12 / GalacticConstants::Parsec) + activeLog10ProperVelocityC;
            double persistenceDistanceParsecs = 0.0;

            if (relativisticVisualsEnabled && std::isfinite(log10PersistencePc))
                persistenceDistanceParsecs = std::pow(10.0, std::clamp(log10PersistencePc, -4.0, 5.3979400086720375));
            glm::vec3 relativisticParams(relativisticVisualsEnabled ? 1.0f : 0.0f, relativisticGamma, static_cast<float>(persistenceDistanceParsecs));
            glm::vec3 viewportInfo(static_cast<float>(width), static_cast<float>(height), 0.0f);

            glm::dvec3 cameraGalacticParsecs = GalacticFrame::LocalMetersToGalactocentricParsecs(camera.Position);
            glm::vec3 cameraGalacticHigh = glm::vec3(cameraGalacticParsecs);
            glm::vec3 cameraGalacticLow = glm::vec3(cameraGalacticParsecs - glm::dvec3(cameraGalacticHigh));
            glm::mat4 densityView = glm::translate(view, -cameraGalacticHigh);

            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            localStarShader.Use();
            localStarShader.SetMat4("view", view);
            localStarShader.SetMat4("projection", starProjection);
            localStarShader.SetVec3("cameraOffsetHigh", cameraGalacticHigh);
            localStarShader.SetVec3("cameraOffsetLow", cameraGalacticLow);
            localStarShader.SetVec3("relativisticParams", relativisticParams);
            localStarShader.SetVec3("observerVelocityDirection", observerVelocityDirection);
            localStarShader.SetVec3("viewportInfo", viewportInfo);
            localMilkyWayStarField.Render();

            double cameraAndromedaDistancePc = glm::length(cameraGalacticParsecs - AndromedaModel::CenterParsecs());
            if (cameraAndromedaDistancePc < 45000.0)
            {
                localAndromedaStarShader.Use();
                localAndromedaStarShader.SetMat4("view", view);
                localAndromedaStarShader.SetMat4("projection", starProjection);
                localAndromedaStarShader.SetVec3("cameraOffsetHigh", cameraGalacticHigh);
                localAndromedaStarShader.SetVec3("cameraOffsetLow", cameraGalacticLow);
                localAndromedaStarShader.SetVec3("relativisticParams", relativisticParams);
                localAndromedaStarShader.SetVec3("observerVelocityDirection", observerVelocityDirection);
                localAndromedaStarShader.SetVec3("viewportInfo", viewportInfo);
                localAndromedaStarField.Render();
            }

            starShader.Use();
            starShader.SetMat4("view", view);
            starShader.SetMat4("projection", starProjection);
            starShader.SetVec3("relativisticParams", relativisticParams);
            starShader.SetVec3("observerVelocityDirection", observerVelocityDirection);
            starShader.SetVec3("viewportInfo", viewportInfo);
            starShader.SetVec3("cameraOffsetHigh", glm::vec3(0.0f));
            starShader.SetVec3("cameraOffsetLow", glm::vec3(0.0f));
            starShader.SetVec3("localFieldParams", glm::vec3(0.0f));
            starShader.SetInt("magnitudeMode", 0);
            starField.Render(camera.Position);

            glDisable(GL_BLEND);

            andromedaDensityShader.Use();
            andromedaDensityShader.SetMat4("view", densityView);
            andromedaDensityShader.SetMat4("projection", galaxyProjection);
            andromedaDensityShader.SetVec3("relativisticParams", relativisticParams);
            andromedaDensityShader.SetVec3("observerVelocityDirectionView", observerVelocityDirectionView);
            andromedaDensityRenderer.Render();
            andromedaDensityRenderer.RenderClouds();
            andromedaDensityRenderer.RenderDust();

            milkyWayDensityShader.Use();
            milkyWayDensityShader.SetMat4("view", densityView);
            milkyWayDensityShader.SetMat4("projection", galaxyProjection);
            milkyWayDensityShader.SetVec3("relativisticParams", relativisticParams);
            milkyWayDensityShader.SetVec3("observerVelocityDirectionView", observerVelocityDirectionView);
            milkyWayDensityRenderer.RenderClouds();
            milkyWayDensityRenderer.RenderDust();

            galacticCenterShader.Use();
            galacticCenterShader.SetMat4("view", view);
            galacticCenterShader.SetMat4("projection", projection);
            galacticCenterMarker.Render(camera.Position);

            namedStarShader.Use();
            namedStarShader.SetMat4("view", view);
            namedStarShader.SetMat4("projection", projection);
            namedStarField.Render(camera.Position);

            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);

            for (std::size_t i = 0; i < bodies.size(); i++)
                sphereRenderer.Render(bodies[i], camera, bodyShader, projection);

            gravitationalLensingRenderer.EndAndRender(bodies, camera.Position, view, projection, width, height);

            hud.Draw(camera, bodies, namedStarField, simulationClock, observer, rocket, observerGravitySystem, view, projection, width, height);

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
