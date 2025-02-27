#include <iostream>

#include "glad/glad.h"

#include <GLFW/glfw3.h>

#include "input_handler.h"

namespace {
constexpr int HEIGHT = 800;
constexpr int WIDTH = 1000;

void GLAPIENTRY MessageCallback(
    GLenum /* source */,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei /* length */,
    const GLchar* message,
    const void* /* userParam */ )
{
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
  std::cerr << "GL CALLBACK: " << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "") 
            << " type = " << type << ", severity = " << severity 
            << ", message = " << message << std::endl;
}

} // namespace

int main() { // int argc, char** argv
  if (!glfwInit())
    return -1;

  GLFWwindow* const window = glfwCreateWindow(WIDTH, HEIGHT, "Simple RayTracer", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    std::cerr << "Couldn't create window\n";
    return -1;
  }

  glfwMakeContextCurrent(window);

  if (!glfwGetCurrentContext()) {
    std::cerr << "Failed to create OpenGL context!" << std::endl;
    return -1;
  }

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD!" << std::endl;
    return -1;
  }

  glEnable(GL_DEBUG_OUTPUT);
  // Required OpenGL >= 4.3
  if (!glDebugMessageCallback) {
    std::cout << "Bad OpenGL version\n";
    return 0;
  }

  glDebugMessageCallback(MessageCallback, nullptr);

  NameMe::InputHandler i_handler(window);
  glfwSetWindowUserPointer(window, &i_handler);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  
  // could std bind these, but this is probably a bit cleaner
  glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
    auto* const handler = static_cast<NameMe::InputHandler*>(glfwGetWindowUserPointer(window));
    handler->MouseMovement(xpos, ypos);
  });

  glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* const handler = static_cast<NameMe::InputHandler*>(glfwGetWindowUserPointer(window));
    handler->OneTimeKeyPressed(key, scancode, action, mods);
  });

  glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
      std::cerr << "OpenGL error: " << err << std::endl;

    i_handler.PollKeys();
    // render();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
