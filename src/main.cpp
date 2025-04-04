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
#include "asset_utils/gpu_loader.h"
#include "asset_utils/model_loader.h"

#include <vector>
#include <glm/gtc/type_ptr.hpp>

namespace
{

  // Vertex and Frag shader source code for the full screen render

  const char *vertexShaderSource = R"(
#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main () {
  gl_Position = vec4(aPos, 1.0);
  TexCoord = aTexCoord;
  }
)";

  const char *fragmentShaderSource = R"(
#version 420 core
out vec4 FragColor;

in vec2 TexCoord;

layout(binding = 0) uniform sampler2D raytraceTexture;

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
    if (vertexShader == 0)
    {
      std::cerr << "Vertex shader compilation failed\n";
      return 0;
    }
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0)
    {
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
  GLuint noiseTBOs[2], noiseTex[2];
  GLuint quadShaderProgram = 0;
  GLuint rayTracerTextureHandle = 0;
  GLuint accumBufferTextureHandle = 0;

  // Flags
  constexpr bool RUN_COMPUTE_RT = true;
  constexpr bool RUN_RT = true;
  constexpr bool REND_TO_TEX = true;
  constexpr bool SHOW_MODEL = false;
  constexpr int HEIGHT = 800;
  constexpr int WIDTH = 1000;
  constexpr int MAX_LIGHTS = 10;

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

  void UpdateLightsWithCamera(std::vector<RayTracer::PointLight> &lights, const RayTracer::Camera &camera)
  {
    // Clear existing lights
    lights.clear();

    // Get camera position and orientation vectors
    glm::vec3 camPos = camera.getOrigin();
    glm::vec3 camForward = camera.getForward();
    glm::vec3 camRight = camera.getRightVector();
    glm::vec3 camUp = camera.getUpVector();

    // Add lights that follow camera orientation, not just position

    // Main light - placed in front based on camera direction
    lights.emplace_back(camPos + camForward * (-4.0f) + camUp * 2.0f,
                        glm::vec3(1.0f, 1.0f, 1.0f), 60.0f);

    // Left and right lights using camera's right vector
    lights.emplace_back(camPos + camRight * (-3.0f) + camUp * 2.0f,
                        glm::vec3(0.8f, 0.8f, 1.0f), 30.0f);
    lights.emplace_back(camPos + camRight * 3.0f + camUp * 2.0f,
                        glm::vec3(1.0f, 0.8f, 0.8f), 30.0f);

    // Top light using camera's up vector
    lights.emplace_back(camPos + camUp * 4.0f,
                        glm::vec3(1.0f, 1.0f, 0.8f), 40.0f);

    // Ambient light behind the camera to soften shadows
    lights.emplace_back(camPos + camForward * 4.0f,
                        glm::vec3(0.4f, 0.4f, 0.5f), 20.0f);
  }

} 

void InitCompute(Graphics::Compute &compute)
{
  compute.Init();

  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR)
    std::cerr << "Compute Init Error: " << err << std::endl;

  std::vector<AssetUtils::Model *> models;
  auto model = AssetUtils::LoadObject("Rubik");
  models.push_back(model.get());
  /*auto plane_model = AssetUtils::LoadObject("11803_Airplane_v1_l1");
  models.push_back(plane_model.get());*/
  AssetUtils::UploadModelDataToGPU(models, 5);

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
  std::cout << "Setting up quad graphics..." << std::endl;

  // Clean up any existing resources first
  if (quadVAO != 0)
  {
    glDeleteVertexArrays(1, &quadVAO);
    quadVAO = 0;
  }
  if (quadVBO != 0)
  {
    glDeleteBuffers(1, &quadVBO);
    quadVBO = 0;
  }
  if (quadShaderProgram != 0)
  {
    glDeleteProgram(quadShaderProgram);
    quadShaderProgram = 0;
  }

  // Create shader program
  quadShaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
  if (quadShaderProgram == 0)
  {
    std::cerr << "CRITICAL ERROR: Quad shader program creation failed!" << std::endl;
    return;
  }

  // Create VAO
  glGenVertexArrays(1, &quadVAO);
  glBindVertexArray(quadVAO);

  // Print VAO ID for debugging
  //std::cout << "Created quadVAO: " << quadVAO << std::endl;

  // Create VBO
  glGenBuffers(1, &quadVBO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

  // Set attributes
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));

  // Important: Keep VAO bound
  // Don't unbind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void UpdateNoiseTex(std::vector<glm::vec3> &noiseData, std::vector<glm::vec3> &noiseUniformData)
{
  for (auto &vec : noiseData)
  {
    vec = Common::randomUnitVector();
  }

  for (auto &vec2 : noiseUniformData)
  {
    vec2 = Common::randomVec3(0.0f, 1.0f);
  }

  int dataSize = WIDTH * HEIGHT * 3 * sizeof(float);

  glGenBuffers(2, noiseTBOs);
  glGenTextures(2, noiseTex);

  glBindBuffer(GL_TEXTURE_BUFFER, noiseTBOs[0]);
  glBufferData(GL_TEXTURE_BUFFER, dataSize, noiseData.data(), GL_DYNAMIC_DRAW);
  glBindTexture(GL_TEXTURE_BUFFER, noiseTex[0]);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, noiseTBOs[0]);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);

  glBindBuffer(GL_TEXTURE_BUFFER, noiseTBOs[1]);
  glBufferData(GL_TEXTURE_BUFFER, dataSize, noiseUniformData.data(), GL_DYNAMIC_DRAW);
  glBindTexture(GL_TEXTURE_BUFFER, noiseTex[1]);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, noiseTBOs[1]);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);

  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR)
    std::cerr << "Bind Noise Buffer: " << err << std::endl;
}

void RenderQuad()
{
  // Re-verify program is active
  GLint currentProgram = 0;
  glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
  if (currentProgram != quadShaderProgram)
  {
    std::cerr << "ERROR: Program not active in RenderQuad(), manually binding..." << std::endl;
    glUseProgram(quadShaderProgram);
  }

  // Verify VAO
  if (quadVAO == 0)
  {
    std::cerr << "ERROR: quadVAO is 0! Cannot render quad" << std::endl;
    return;
  }

  // Activate texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, rayTracerTextureHandle);

  // Store previous VAO state
  GLint previousVAO = 0;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);

  // Bind our VAO
  glBindVertexArray(quadVAO);

  // Verify binding succeeded
  GLint boundVAO = 0;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVAO);
  if (boundVAO != quadVAO)
  {
    std::cerr << "ERROR: VAO binding failed! Expected: " << quadVAO << " Got: " << boundVAO << std::endl;
    return;
  }

  // Draw
  glDrawArrays(GL_TRIANGLES, 0, 6);

  // Restore previous VAO
  if (previousVAO != 0)
  {
    glBindVertexArray(previousVAO);
  }
}

void CleanupQuad()
{
  glDeleteVertexArrays(1, &quadVAO);
  glDeleteBuffers(1, &quadVBO);
  glDeleteProgram(quadShaderProgram);
}

void ValidateRenderState()
{
  // Check if VAO exists
  if (quadVAO == 0)
  {
    std::cerr << "CRITICAL: quadVAO is 0, recreating quad setup" << std::endl;
    SetupQuad();
  }

  // Check if quad shader program exists
  if (quadShaderProgram == 0)
  {
    std::cerr << "CRITICAL: quadShaderProgram is 0, recreating quad setup" << std::endl;
    SetupQuad();
  }

  // Check if texture exists
  if (rayTracerTextureHandle == 0)
  {
    std::cerr << "CRITICAL: rayTracerTextureHandle is 0" << std::endl;
  }
}

int main()
{ // int argc, char** argv
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *const window = glfwCreateWindow(WIDTH, HEIGHT, "Simple RayTracer", nullptr, nullptr);
  if (!window)
  {
    glfwTerminate();
    std::cerr << "Couldn't create window\n";
    return -1;
  }

  glfwMakeContextCurrent(window);

  if (!glfwGetCurrentContext())
  {
    std::cerr << "Failed to create OpenGL context!" << std::endl;
    return -1;
  }

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cerr << "Failed to initialize GLAD!" << std::endl;
    return -1;
  }

  glEnable(GL_DEBUG_OUTPUT);
  // Required OpenGL >= 4.3
  if (!glDebugMessageCallback)
  {
    std::cout << "Bad OpenGL version\n";
    return 0;
  }

  glDebugMessageCallback(MessageCallback, nullptr);

  // std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << "\n";
  // std::cout << "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n";
  // std::cout << "GLSL_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
  glDisable(GL_DEPTH_TEST);

  std::cout << "GL_RENDERER: " << glGetString(GL_RENDERER) << std::endl;
  std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;
  // Set up our full screen quad
  SetupQuad();

  // Define the camera
  RayTracer::CameraSettings settings;
  settings.aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
  settings.width = WIDTH;
  settings.samplesPerPixel = 1;
  settings.maxDepth = 2;

  RayTracer::Camera camera(settings);
  camera.Initialize();

  // Set up the input handler
  InputHandler inputHandler(window, camera);
  glfwSetWindowUserPointer(window, &inputHandler);

  // Hide cursor but don't capture it
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  // Make sure mouse capture is disabled
  inputHandler.EnableMouseCapture(false);

  // Input callbacks
  glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos)
                           {
    auto* const handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    handler->MouseCallback(window, xpos, ypos); });

  glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                     {
    auto* const handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    handler->KeyCallback(window, key, scancode, action, mods); });

  Graphics::Compute compute("./shaders/raytrace_compute.glsl");
  Graphics::Texture texture;
  Graphics::Texture32 accumBuffer;
  std::vector<glm::vec3> noiseData(WIDTH * HEIGHT);
  std::vector<glm::vec3> noiseDataUniform(WIDTH * HEIGHT);
  if (RUN_COMPUTE_RT)
  {
    // Create a texture with proper format for both compute writing and fragment sampling
    texture.setWidth(WIDTH);
    texture.setHeight(HEIGHT);

    Graphics::Color8 defaultColor = {0, 0, 0};
    std::vector<Graphics::Color8> blankData(WIDTH * HEIGHT, defaultColor);

    // Initialize with actual data first
    texture.Init(blankData, WIDTH, HEIGHT);

    // Get handle AFTER initialization
    rayTracerTextureHandle = texture.getTextureHandle(GL_TEXTURE0, 0, true);

    // Configure texture immediately after getting handle
    glBindTexture(GL_TEXTURE_2D, rayTracerTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    accumBuffer.setWidth(WIDTH);
    accumBuffer.setHeight(HEIGHT);
    UpdateNoiseTex(noiseData, noiseDataUniform);
    accumBufferTextureHandle = accumBuffer.getTextureHandle(GL_TEXTURE3, 3, true);
    InitCompute(compute);
  }
  // Run the Raytracer
  else if (RUN_RT)
  {
    // Define the camera
    RayTracer::CameraSettings settings;
    settings.aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    settings.width = WIDTH;
    settings.samplesPerPixel = 1;
    settings.maxDepth = 5;

    RayTracer::Camera camera(settings);
    camera.Initialize();

    // Set up the input handler
    InputHandler inputHandler(window, camera);
    glfwSetWindowUserPointer(window, &inputHandler);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide the cursor and capture it

    // Input callbacks
    glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos)
                             {
          auto* const handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
          handler->MouseCallback(window, xpos, ypos); });

    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                       {
          auto* const handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
          handler->KeyCallback(window, key, scancode, action, mods); });

    RayTracer::World world;
    SetupWorld(world);

    std::vector<RayTracer::PointLight> lights;
    lights.reserve(MAX_LIGHTS);
    lights.emplace_back(glm::vec3(1.0, 2.0, 0.0), glm::vec3(1, 1, 1), 5.0f);

    RayTracer::RayTracer raytracer(settings, "image.ppm", REND_TO_TEX);
    raytracer.Init(world);
    raytracer.Render();

    if (REND_TO_TEX)
    {
      texture = raytracer.getTexture();
      // Get the texture handle once and store it
      // Note: This calls the method that creates the texture
      // We're casting away const because the method isn't const
      rayTracerTextureHandle = texture.getTextureHandle(GL_TEXTURE0, 0);
      if (rayTracerTextureHandle == 0)
      {
        std::cerr << "Error: rayTracerTextureHandle is 0" << std::endl;
        return -1;
      }
      texture.debugReadTexture();
    }
  }
  else
  {
    // Create a simple test texture
    std::vector<Graphics::Color8> pixels(WIDTH * HEIGHT);
    for (int y = 0; y < HEIGHT; y++)
    {
      for (int i = 0; i < WIDTH * HEIGHT; i++)
      {
        // red
        pixels[i] = Graphics::Color8{(Graphics::byte)y, 20, 20};
      }
    }

    texture.Init(pixels, WIDTH, HEIGHT);

    // Get the texture handle
    rayTracerTextureHandle = texture.getTextureHandle(GL_TEXTURE0, 0);
    if (rayTracerTextureHandle == 0)
    {
      std::cerr << "Error: rayTracerTextureHandle is 0" << std::endl;
      return -1;
    }
  }

  int accumFrames = 0;
  std::vector<RayTracer::PointLight> lights;
  lights.reserve(MAX_LIGHTS);
  if (SHOW_MODEL)
  {
    lights.emplace_back(glm::vec3(1.0, 10.0, 10.0), glm::vec3(1.0, 1.0, 1.0), 50.0f);
    lights.emplace_back(glm::vec3(-5.0, 15.0, 10.0), glm::vec3(1.0, 0.2, 0.2), 15.0f);
    lights.emplace_back(glm::vec3(5.0, 15.0, 10.0), glm::vec3(0.2, 1.0, 0.2), 15.0f);
    lights.emplace_back(glm::vec3(-5.0, 5.0, 10.0), glm::vec3(0.2, 0.2, 1.0), 15.0f);
    lights.emplace_back(glm::vec3(5.0, 5.0, 10.0), glm::vec3(1.0, 1.0, 0.1), 15.0f);
    lights.emplace_back(glm::vec3(0.0, 21.0, 17.0), glm::vec3(1.0, 1.0, 1.0), 50.0f);
  }
  else
  {
    lights.emplace_back(glm::vec3(1.0, 2.0, 0.0), glm::vec3(1.0, 1.0, 1.0), 10.0f);
    lights.emplace_back(glm::vec3(-2.5, 2.0, 0.0), glm::vec3(1.0, 1.0, 1.0), 3.0f);
  }

  glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
  static float lastFrameTime = 0.0f;
  static int frameCounter = 0;
  while (!glfwWindowShouldClose(window))
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
      std::cerr << "OpenGL error: " << err << std::endl;

    float currentFrameTime = glfwGetTime();
    float deltaTime = currentFrameTime - lastFrameTime;
    lastFrameTime = currentFrameTime;

    // IMPORTANT: Process input first thing every frame
    inputHandler.ProcessInput(deltaTime);

    // Debug frame rate less frequently
    if (++frameCounter % 60 == 0)
    {
      std::cout << "Frame Time: " << deltaTime * 1000.0f << " ms" << std::endl;
      frameCounter = 0;
    }

    bool resetBuffer = false;
    static bool wasAnyInput = false;
    static int noInputFrames = 0;
    int maxAccumFrames = 32; // Reduced to prevent overexposure, causes the 'white' buildup when camera is still

    // Get input state
    glm::vec3 movementDelta = inputHandler.GetMovementDelta();
    glm::vec2 rotationDelta = inputHandler.GetRotationDelta();

    // Check for any active input
    bool anyInput =
        glm::length(movementDelta) > 0.0001f ||
        glm::length(rotationDelta) > 0.0001f ||
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    // Detect input state changes
    if (anyInput)
    {
      resetBuffer = true;
      accumFrames = 0;
      noInputFrames = 0;
      wasAnyInput = true;
    }
    else
    {
      // No input this frame
      noInputFrames++;

      if (wasAnyInput && noInputFrames <= 5)
      {
        // Just stopped input - reset for a few more frames to stabilize
        resetBuffer = true;
        accumFrames = 0;
      }
      else if (accumFrames < maxAccumFrames)
      {
        // Continue accumulating up to the maximum
        resetBuffer = false;
        accumFrames++;
      }

      wasAnyInput = false;
    }

    // Add periodic forced reset to prevent any drift issues
    if (frameCounter % 300 == 0)
    {
      resetBuffer = true;
      accumFrames = 0;
      //std::cout << "Periodic buffer reset at frame: " << frameCounter << std::endl;
    }

    // Also reset if requested by input handler
    if (inputHandler.ShouldResetBuffer())
    {
      resetBuffer = true;
      accumFrames = 0;
      inputHandler.ClearResetFlag();
    }

    // Move the camera
    float movementSpeed = 10.0f;
    camera.MoveAndRotate(deltaTime, movementDelta, rotationDelta, movementSpeed);

    // Update lights to move with camera EVERY FRAME for responsive lighting, can potentially modify this for static lights
    // This is a critical function that should be called every frame to ensure lights are updated correctly
    UpdateLightsWithCamera(lights, camera);

    if (RUN_COMPUTE_RT)
    {
      // First store the current VAO state before compute operations
      GLint previousVAO = 0;
      glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVAO);

      // Ensure compute shader is active before using it
      compute.Use();

      // IMPORTANT: Explicitly set resetAccumBuffer every frame
      compute.SetBool("resetAccumBuffer", resetBuffer);

      // IMPORTANT: Always update camera data every frame regardless of input
      compute.SetVec3("cameraOrigin", camera.getOrigin());
      compute.SetVec3("cameraDirection", camera.getForward());
      compute.SetVec3("cameraUp", camera.getUpVector());
      compute.SetVec3("cameraRight", camera.getRightVector());

      // Set accumFrames after potential reset
      compute.SetInt("accumFrames", accumFrames);

      // Set other parameters
      compute.SetInt("Width", WIDTH);
      compute.SetInt("Height", HEIGHT);
      compute.SetUInt("bvh_count", 2); // models in scene
      compute.SetInt("lightCount", lights.size());

      // Force buffer reset periodically regardless of other conditions
      if (frameCounter % 1200 == 0)
      {
        compute.SetBool("resetAccumBuffer", true);
        compute.SetInt("accumFrames", 0);
        //std::cout << "Forced compute shader buffer reset" << std::endl;
      }

      // Create SSBO for lights with refreshed data every frame
      GLuint ssbo;
      glGenBuffers(1, &ssbo);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
      glBufferData(GL_SHADER_STORAGE_BUFFER, lights.size() * sizeof(RayTracer::PointLight), lights.data(), GL_DYNAMIC_DRAW);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo);

      // IMPORTANT: Bind textures but DON'T try to set uniform locations manually
      // Since compute shader uses layout(binding = X) syntax
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_BUFFER, noiseTex[0]);

      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_BUFFER, noiseTex[1]);

      // Bind image for writing
      glBindImageTexture(0, rayTracerTextureHandle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

      // Dispatch compute shader
      glDispatchCompute((unsigned int)(WIDTH / 8), (unsigned int)(HEIGHT / 8), 1);

      // Complete memory barrier to ensure all writes are finished
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                      GL_TEXTURE_FETCH_BARRIER_BIT |
                      GL_SHADER_STORAGE_BARRIER_BIT);

      // Clean up compute resources BUT DON'T unbind VAO
      glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
      glDeleteBuffers(1, &ssbo);

      // Force OpenGL to finish operations
      glFinish();

      // Restore previous VAO binding if it was valid
      if (previousVAO != 0)
      {
        glBindVertexArray(previousVAO);
      }
    }

    // IMPORTANT: Reset all state completely
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);

    // Clear any pending errors
    while ((err = glGetError()) != GL_NO_ERROR)
    {
      std::cerr << "OpenGL error before program switch: " << err << std::endl;
    }

    // Validate render state before rendering
    ValidateRenderState();

    // Now bind the rendering program
    glUseProgram(quadShaderProgram);

    // Debug validation
    GLint currentProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    if (currentProgram != quadShaderProgram)
    {
      std::cerr << "CRITICAL ERROR: Failed to bind quad program!" << std::endl;
      return -1;
    }

    // Only render if texture is valid
    if (rayTracerTextureHandle != 0)
    {
      RenderQuad();
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Clean up

  CleanupQuad();
  glDeleteTextures(2, noiseTex);
  glDeleteBuffers(2, noiseTBOs);

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}