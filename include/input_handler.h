#pragma once

#include <GLFW/glfw3.h>

namespace NameMe {
class InputHandler {
 public:
  InputHandler(GLFWwindow* const window) : window_(window) {}

  void MouseMovement(const double /* xpos */, const double /* ypos */) {}

  /**
   * For Rarer single time key presses
   * 
   * Since this is event driven it's not great of continous stuff
   */
  void OneTimeKeyPressed(
      const int key,
      const int /* scancode */,
      const int action,
      const int /* mods */) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window_, GLFW_TRUE);
  }

  /**
   * Poll based key approach.
   * 
   * Use for per frame stuff. Probably movement.
   */
  void PollKeys() {
    // if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  }

 private:
  GLFWwindow* const window_;
};

}  // namespace NameMe
