#pragma once

#include <GLFW/glfw3.h>
#include "raytracer/camera.h"

class InputHandler {
public:
    InputHandler(GLFWwindow* window, RayTracer::Camera& camera)
        : window(window), camera(camera), lastX(0.0), lastY(0.0), firstMouse(true) {
        // glfwSetWindowUserPointer(window, this);
        // glfwSetKeyCallback(window, KeyCallback);
        // glfwSetCursorPosCallback(window, MouseCallback);
    }

    void ProcessInput(float deltaTime);

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    glm::vec3 GetMovementDelta() const;
    glm::vec2 GetRotationDelta() const;

private:
    GLFWwindow* window;
    RayTracer::Camera& camera;
    float lastX, lastY;
    bool firstMouse;
    glm::vec3 movementDelta; // movement accumulation
    glm::vec2 rotationDelta;
};