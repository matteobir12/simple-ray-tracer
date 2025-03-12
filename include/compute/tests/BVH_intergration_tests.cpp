#include <gtest/gtest.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "intersection_utils/bvh.h"
#include "common/types.h"
#include "compute/create_compute_program.h"
#include "asset_utils/gpu_loader.h"
#include "asset_utils/model_loader.h"

#include <vector>

namespace Compute {
namespace Testing {
namespace {
 constexpr const char* ComputeShaderPath = "./shaders/ray_intersects.glsl";
}

class IntigrationTestStruct : public testing::Test  {
 public:
  virtual ~IntigrationTestStruct() {};
  IntigrationTestStruct() {
    glfwInit();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // offscreen
    GLFWwindow* const window = glfwCreateWindow(640, 480, "Compute Test", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    compute_prog = CreateComputeProgram(ComputeShaderPath);

    glGenBuffers(1, &hit_buffer_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, hit_buffer_);
    const GLint hit_count = 64;
    GLint hit_size = hit_count * sizeof(GLuint);

    std::vector<GLuint> zero_hits(hit_count, (std::uint32_t) -1);
    glBufferData(GL_SHADER_STORAGE_BUFFER, hit_size, zero_hits.data(), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, hit_buffer_);

    glUseProgram(compute_prog);
  }

  std::vector<GLuint> GetHits() {
    const GLint hit_count = 64;
    std::vector<GLuint> out(hit_count, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, hit_buffer_);
    // glMapBufferRange also exist, but doesn't matter for test code
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, hit_count * sizeof(GLuint), out.data());

    return out;
  }

  GLuint hit_buffer_ = 0;
  GLuint compute_prog = 0;
};

TEST_F(IntigrationTestStruct, Test) {
  const std::string OBJ_loc = "Rubik";
  const auto out = AssetUtils::LoadObject(OBJ_loc);
  AssetUtils::UploadModelDataToGPU({out.get()});
  const GLint loc = glGetUniformLocation(compute_prog, "bvh_count");
  glUniform1ui(loc, 1);
  std::vector<Common::Ray> rays;
  rays.reserve(64);
  // Odd rays hit, even miss
  for (int i = 0; i < 64; ++i) {
    if (i % 2)
      rays.emplace_back(glm::vec3{-10.0, 3.0, 6.0}, glm::vec3{0.9838, -0.0118, 0.1787});
    else
      rays.emplace_back(glm::vec3{-10.0, 3.0, 6.0}, glm::vec3{0., 1, 0});
  }

  AssetUtils::UpdateRays(64, rays.data());
  // has local_size_x=8 and local_size_y=8.
  // if we want 8x8 threads total we do 1 group in x and 1 group in y.
  glUseProgram(compute_prog);
  glDispatchCompute(1, 1, 1);
  glMemoryBarrier(GL_ALL_BARRIER_BITS);

  GLenum err;
  while((err = glGetError()) != GL_NO_ERROR)
    std::cerr << "OpenGL error: " << err << std::endl;

  const auto hits = GetHits();
  ASSERT_EQ(hits.size(), 64);
  // All even rays should miss and all odd should hit
  for (std::size_t i = 0; i < hits.size(); ++i)
    EXPECT_EQ(hits[i], i % 2);
}

}
}