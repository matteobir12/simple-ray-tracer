#include "input_handler.h"
#include <iostream>

void InputHandler::ProcessInput(float deltaTime) {
    movementDelta = glm::vec3(0.0f);
    rotationDelta = glm::vec2(0.0f);

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

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f; 
    rotationDelta.x = xOffset * sensitivity; 
    rotationDelta.y = yOffset * sensitivity; 
}

void InputHandler::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true); // Close window
        }
    }
}

void InputHandler::MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    InputHandler* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (handler->firstMouse) {
            handler->lastX = xpos;
            handler->lastY = ypos;
            handler->firstMouse = false;
        }
    

    float xoffset = xpos - handler->lastX;
    float yoffset = handler->lastY - ypos; // Reversed since y-coordinates go from bottom to top

    handler->lastX = xpos;
    handler->lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    handler->camera.Rotate(xoffset, yoffset);
    }
    else {
        handler->firstMouse = true; 
    }
}

glm::vec3 InputHandler::GetMovementDelta() const {
    return movementDelta;
}

glm::vec2 InputHandler::GetRotationDelta() const {
    return rotationDelta;
}