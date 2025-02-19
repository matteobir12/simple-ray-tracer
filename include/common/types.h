#pragma once

#include <stdexcept>
#include <utility>

#include <glm/vec3.hpp>

namespace Common {
struct Triangle {
  glm::vec3 v0;
  glm::vec3 v1;
  glm::vec3 v2;

  Triangle(glm::vec3 _v0, glm::vec3 _v1, glm::vec3 _v2)
    : v0(_v0), v1(_v1), v2(_v2) {}

  glm::vec3& operator[](const int idx) {
    if (idx < 0 && idx > 2)
      throw std::out_of_range("Tried to fetch triangle vertex: " + idx);
    
    if (idx == 0)
      return v0;

    if (idx == 1)
      return v1;
    
    return v2;
  }

  static glm::vec3 Centroid(const Triangle& tri) {
    return (tri.v0 + tri.v1 + tri.v2) * 0.3333f;
  }

  static std::pair<glm::vec3, glm::vec3> Bounds(const Triangle& tri) {
    std::pair<glm::vec3, glm::vec3> out = std::make_pair(tri.v0, tri.v0);
    out.first = glm::min(out.first, tri.v1);
    out.first = glm::min(out.first, tri.v2);
    out.second = glm::max(out.second, tri.v1);
    out.second = glm::max(out.second, tri.v2);

    return out;
  }
};

struct Ray {
  glm::vec3 origin;

  /**
   * Expected to be normalized
   */
  glm::vec3 direction;
  float intersection_distance = 1e30f;
};

}  // namespace Common
