#pragma once

#include <GLFW/glfw3.h>
#include "raytracer/camera.h"

class InputHandler
{
public:
    InputHandler(GLFWwindow *window, RayTracer::Camera &camera)
        : window(window), camera(camera), lastX(0.0), lastY(0.0),
          firstMouse(true), movementDelta(0.0f), rotationDelta(0.0f),
          shouldResetBuffer(false), mouseCaptured(false)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    void EnableMouseCapture(bool enable);
    void ProcessInput(float deltaTime);
    static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void MouseCallback(GLFWwindow *window, double xpos, double ypos);
    glm::vec3 GetMovementDelta() const;
    glm::vec2 GetRotationDelta() const;
    bool ShouldResetBuffer() const;
    void ClearResetFlag();

private:
    GLFWwindow *window;
    RayTracer::Camera &camera;
    float lastX, lastY;
    bool firstMouse;
    glm::vec3 movementDelta{0.0f};
    glm::vec2 rotationDelta{0.0f};
    bool shouldResetBuffer = false;
    bool mouseCaptured = false;
};