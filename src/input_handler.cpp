#include "input_handler.h"
#include <iostream>

void InputHandler::ProcessInput(float deltaTime)
{
    // Process keyboard input for movement
    movementDelta = glm::vec3(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        movementDelta.z += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        movementDelta.z -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        movementDelta.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        movementDelta.x += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        movementDelta.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        movementDelta.y -= 1.0f;

    // Check for left mouse button state to control rotation
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
    {
        // Clear rotation when mouse button is not pressed
        rotationDelta = glm::vec2(0.0f);
    }
}

void InputHandler::KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    InputHandler *handler = static_cast<InputHandler *>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            glfwSetWindowShouldClose(window, true); 
        }
        else if (key == GLFW_KEY_TAB)
        {
            // Toggle mouse capture mode
            handler->mouseCaptured = !handler->mouseCaptured;

            if (handler->mouseCaptured)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                handler->firstMouse = true;
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }

            // Always reset buffer when changing modes
            handler->shouldResetBuffer = true;
        }
        else if (key == GLFW_KEY_R)
        {
            // Reset camera to default position
            handler->camera.Reset();
            handler->shouldResetBuffer = true;
            std::cout << "Camera reset to initial position" << std::endl;

            // Print camera debug info - Use getOrigin() instead of getPosition()
            glm::vec3 pos = handler->camera.getOrigin();
            glm::vec3 dir = handler->camera.getForward();
            std::cout << "Camera position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
            std::cout << "Camera direction: (" << dir.x << ", " << dir.y << ", " << dir.z << ")" << std::endl;
        }
        else if (key == GLFW_KEY_L)
        {
            // Toggle debug overlay
            handler->shouldResetBuffer = true;
            std::cout << "Camera position: " << handler->camera.getOrigin().x << ", " << handler->camera.getOrigin().y << ", " << handler->camera.getOrigin().z << std::endl;
        }
    }
}

// Store previous position as class member for stability
void InputHandler::MouseCallback(GLFWwindow *window, double xpos, double ypos)
{
    InputHandler *handler = static_cast<InputHandler *>(glfwGetWindowUserPointer(window));

    // Only handle in mouse button mode
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        // First mouse press this session
        if (handler->firstMouse)
        {
            handler->lastX = xpos;
            handler->lastY = ypos;
            handler->firstMouse = false;
            handler->rotationDelta = glm::vec2(0.0f, 0.0f);
            return;
        }

        // Calculate movement delta
        double xoffset = xpos - handler->lastX;
        double yoffset = handler->lastY - ypos; // Inverted y-axis

        // Update position for next frame
        handler->lastX = xpos;
        handler->lastY = ypos;

        // Detect erratic movements (cursor warping, etc)
        if (std::abs(xoffset) > 100.0 || std::abs(yoffset) > 100.0)
        {
            handler->rotationDelta = glm::vec2(0.0f, 0.0f);
            return; // Skip this frame
        }

        const float sensitivity = 0.05f;
        handler->rotationDelta.x = static_cast<float>(xoffset * sensitivity);
        handler->rotationDelta.y = static_cast<float>(yoffset * sensitivity);

        // Always reset buffer when rotating
        handler->shouldResetBuffer = true;

        // Periodically recenter cursor (helps avoid drift)
        static int counter = 0;
        if (++counter % 120 == 0 && handler->mouseCaptured)
        {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            glfwSetCursorPos(window, width / 2, height / 2);
            handler->lastX = width / 2;
            handler->lastY = height / 2;
            counter = 0;
        }
    }
    else
    {
        // Reset when button released
        handler->firstMouse = true;
        handler->rotationDelta = glm::vec2(0.0f, 0.0f);
    }
}

glm::vec3 InputHandler::GetMovementDelta() const
{
    return movementDelta;
}

glm::vec2 InputHandler::GetRotationDelta() const
{
    return rotationDelta;
}

bool InputHandler::ShouldResetBuffer() const
{
    return shouldResetBuffer;
}

void InputHandler::ClearResetFlag()
{
    shouldResetBuffer = false;
}

void InputHandler::EnableMouseCapture(bool enable)
{
    mouseCaptured = enable;
    if (mouseCaptured)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    shouldResetBuffer = true;
}