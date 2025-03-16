#include <iostream>
#include <memory>

#include "glad/glad.h"

#include <GLFW/glfw3.h>

#include "input_handler.h"
#include "raytracer/raytracer.h"
#include "raytracer/world.h"
#include "raytracer/light.h"
#include "raytracer/raytracer_types.h"
#include "raytracer/utils.h"
#include "graphics/texture.h"
#include "graphics/shader.h"

#include <vector>
#include <glm/gtc/type_ptr.hpp>

namespace
{

  // Vertex and Frag shader source code for the full screen render

  const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main () {
  gl_Position = vec4(aPos, 1.0);
  TexCoord = aTexCoord;
  }
)";

  const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D raytraceTexture;

void main () {
    FragColor = texture(raytraceTexture, TexCoord);
}
)";

  // Two triangles making a quad covering the whole screen
  const float quadVertices[] = {
      // positions        // texture coords
      -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // top left
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
      1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // bottom right

      -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top left
      1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom right
      1.0f, 1.0f, 0.0f, 1.0f, 1.0f   // top right
  };

  // Compiling a shader
  GLuint compileShader(GLenum type, const char *source)
  {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
      GLchar infoLog[512];
      glGetShaderInfoLog(shader, 512, nullptr, infoLog);
      std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n"
                << infoLog << std::endl;
      return 0;
    }

    return shader;
  }

  // To create a shader program
  GLuint createShaderProgram(const char *vertexSource, const char *fragmentSource)
  {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
        std::cerr << "Vertex shader compilation failed\n";
        return 0;
    }
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        std::cerr << "Fragment shader compilation failed\n";
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check for linking errors
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
      glGetProgramInfoLog(program, 512, nullptr, infoLog);
      std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n"
                << infoLog << std::endl;
      return 0;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
  }

  GLuint quadVAO = 0, quadVBO = 0;
  GLuint noiseTBO = 0, noiseTex = 0;
  GLuint quadShaderProgram = 0;
  GLuint rayTracerTextureHandle = 0;

  constexpr bool RUN_COMPUTE_RT = true;
  constexpr bool RUN_RT = true;
  constexpr bool REND_TO_TEX = true;
  constexpr int HEIGHT = 800;
  constexpr int WIDTH = 1000;

  void GLAPIENTRY MessageCallback(
      GLenum /* source */,
      GLenum type,
      GLuint id,
      GLenum severity,
      GLsizei /* length */,
      const GLchar *message,
      const void * /* userParam */)
  {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
      return;
    std::cerr << "GL CALLBACK: " << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "")
              << " type = " << type << ", severity = " << severity
              << ", message = " << message << std::endl;
  }

} // namespace

void InitCompute(Graphics::Compute& compute, std::vector<glm::vec3>& noiseData) {
    compute.Init();

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
        std::cerr << "Compute Init Error: " << err << std::endl;

    for (auto& vec : noiseData) {
        vec = Common::randomUnitVector();
    }

    int dataSize = WIDTH * HEIGHT * 3 * sizeof(float);
    glGenBuffers(1, &noiseTBO);
    glBindBuffer(GL_TEXTURE_BUFFER, noiseTBO);
    glBufferData(GL_TEXTURE_BUFFER, dataSize, noiseData.data(), GL_DYNAMIC_DRAW);

    while ((err = glGetError()) != GL_NO_ERROR)
        std::cerr << "Bind Noise Buffer: " << err << std::endl;

    glGenTextures(1, &noiseTex);
    glBindTexture(GL_TEXTURE_BUFFER, noiseTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, noiseTBO);

    while ((err = glGetError()) != GL_NO_ERROR)
        std::cerr << "Bind Noise Buffer: " << err << std::endl;

    /*glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, noiseTex);
    glUniform1i(glGetUniformLocation(compute.GetProgram(), "noiseTex"), 0);*/

    while ((err = glGetError()) != GL_NO_ERROR)
        std::cerr << "Bind Noise Buffer: " << err << std::endl;
}

void SetupWorld(RayTracer::World &world)
{
  world.add(std::make_shared<RayTracer::Sphere>(RayTracer::Point3(0.0f, 0.0f, -1.0f), 0.5f));
  world.add(std::make_shared<RayTracer::Sphere>(RayTracer::Point3(0.0f, -100.5f, -1.0f), 100.0f));
}

void SetupQuad()
{
  quadShaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

  if (quadShaderProgram == 0) {
    std::cerr << "Shader program creation failed\n";
    return;
  }

  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);

  glBindVertexArray(quadVAO);

  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // texture coord attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void RenderQuad() {
  // std::cout << "RenderQuad => current context: "
  // << glfwGetCurrentContext() << std::endl;
  // std::cout << "RenderQuad => using texture handle: "
  // << rayTracerTextureHandle << std::endl;

  glUseProgram(quadShaderProgram);
  
  if(rayTracerTextureHandle == 0) {
    std::cerr << "Error: rayTracerTextureHandle is 0 in RenderQuad" << std::endl;
    return;
  }

  // Bind the ray tracer texture using the stored handle
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, rayTracerTextureHandle);
  GLint loc = glGetUniformLocation(quadShaderProgram, "raytraceTexture");
  if (loc == -1) {
      std::cerr << "Uniform 'raytraceTexture' not found" << std::endl;
  }
  glUniform1i(loc, 0);

  // Draw the quad
  glBindVertexArray(quadVAO);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
      std::cerr << "OpenGL error after glDrawArrays: " << err << std::endl;
  }
  glBindVertexArray(0);
}

void CleanupQuad() {
  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glDeleteProgram(quadShaderProgram);
}

int main() { // int argc, char** argv
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

  GLFWwindow* const window = glfwCreateWindow(WIDTH, HEIGHT, "Simple RayTracer", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    std::cerr << "Couldn't create window\n";
    return -1;
  }

  glfwMakeContextCurrent(window);
  std::cout << "Current context: " << glfwGetCurrentContext() << std::endl;

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

  // std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";
  // std::cout << "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n";
  // std::cout << "GLSL_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

  NameMe::InputHandler i_handler(window);
  glfwSetWindowUserPointer(window, &i_handler);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  
  // Input callbacks
  glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
    auto* const handler = static_cast<NameMe::InputHandler*>(glfwGetWindowUserPointer(window));
    handler->MouseMovement(xpos, ypos);
  });

  glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* const handler = static_cast<NameMe::InputHandler*>(glfwGetWindowUserPointer(window));
    handler->OneTimeKeyPressed(key, scancode, action, mods);
  });

  glDisable(GL_DEPTH_TEST);

  
  // Set up our full screen quad
  SetupQuad();

  Graphics::Compute compute("./shaders/raytrace_compute.glsl");
  Graphics::Texture texture;
  std::vector<glm::vec3> noiseData(WIDTH * HEIGHT);
  if (RUN_COMPUTE_RT) {
      texture.setWidth(WIDTH);
      texture.setHeight(HEIGHT);
      rayTracerTextureHandle = texture.getTextureHandle(true);
      InitCompute(compute, noiseData);
  }
  // Run the Raytracer
  else if (RUN_RT) {
      RayTracer::CameraSettings settings;
      settings.aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
      settings.width = WIDTH;
      settings.samplesPerPixel = 10;
      settings.maxDepth = 10;

      RayTracer::World world;
      SetupWorld(world);

      RayTracer::DirectionalLight light(glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1));

      RayTracer::RayTracer raytracer(settings, "image.ppm", REND_TO_TEX);
      raytracer.Init(world);
      raytracer.Render();

      if (REND_TO_TEX)
      {
          texture = raytracer.getTexture();
          // Get the texture handle once and store it
          // Note: This calls the method that creates the texture
          // We're casting away const because the method isn't const
          rayTracerTextureHandle = texture.getTextureHandle();
          texture.debugReadTexture();
      }
  }
  else {
      // Create a simple test texture
      std::vector<Graphics::Color8> pixels(WIDTH * HEIGHT);
      for (int y = 0; y < HEIGHT; y++) {
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
          // red
          pixels[i] = Graphics::Color8{(Graphics::byte)y, 20, 20};
       }
      }
      
      texture.Init(pixels, WIDTH, HEIGHT);
      
      // Get the texture handle 
      rayTracerTextureHandle = texture.getTextureHandle();
  }

  std::vector<glm::vec3> noise = RayTracer::getNoiseBuffer();
  RayTracer::DirectionalLight light(glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1));

  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
      std::cerr << "OpenGL error: " << err << std::endl;

    i_handler.PollKeys();



    if (RUN_COMPUTE_RT) {
        compute.Use();

        compute.SetInt("Width", WIDTH);
        compute.SetInt("Height", HEIGHT);

        glUniform3fv(glGetUniformLocation(compute.GetProgram(), "lightDirection"), 1, glm::value_ptr(light.direction));
        glUniform3fv(glGetUniformLocation(compute.GetProgram(), "lightColor"), 1, glm::value_ptr(light.color));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, noiseTex);
        glUniform1i(glGetUniformLocation(compute.GetProgram(), "noiseTex"), 0);

        glDispatchCompute((unsigned int)WIDTH, (unsigned int)HEIGHT, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    
    // Render the full screen quad with the ray tracer texture
    if (rayTracerTextureHandle != 0) {
        RenderQuad();
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Clean up
  CleanupQuad();
  
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}