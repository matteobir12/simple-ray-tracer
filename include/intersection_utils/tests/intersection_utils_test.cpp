#include <gtest/gtest.h>
#include "intersection_utils/bvh.h"

#include "common/types.h"

namespace IntersectionUtils {
namespace testing {
namespace {
/**
 * For reference, though this will probably be on the GPU
 * 
 * Möller–Trumbore intersection algorithm
 * https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
 */
void RayIntersectsTri(Common::Ray& ray, const Common::Triangle& tri )
{
  const glm::vec3 edge_1 = tri.v1 - tri.v0;
  const glm::vec3 edge_2 = tri.v2 - tri.v0;
  const glm::vec3 h = glm::cross(ray.direction, edge_2);
  const float a = glm::dot(edge_1, h);
  if (a > -0.0001f && a < 0.0001f)
    return; // ray parallel to triangle

  const float f = 1 / a;
  const glm::vec3 s = ray.origin - tri.v0;
  const float u = f * glm::dot(s, h);
  if (u < 0 || u > 1)
    return;

  const glm::vec3 q = glm::cross(s, edge_1);
  const float v = f * glm::dot(ray.direction, q);
  if (v < 0 || u + v > 1)
    return;

  const float t = f * glm::dot( edge_2, q );
  if (t > 0.0001f)
    ray.intersection_distance = std::min(ray.intersection_distance, t);
}
}

TEST(BVHTest, Construction) {
  std::vector<Common::Triangle> triangles;
  triangles.emplace_back(glm::vec3{0, 0, 1}, glm::vec3{0, 1, 0}, glm::vec3{1, 0, 0});
  BVH<Common::Triangle> obj{&triangles, &Common::Triangle::Centroid, &Common::Triangle::Bounds};
}

}
}
