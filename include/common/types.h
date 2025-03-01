#pragma once

#include <stdexcept>
#include <utility>
#include <type_traits>

#include <glm/glm.hpp>
#include <glm/vec3.hpp>

namespace Common {

using uint = uint32_t;
const float infinity = std::numeric_limits<float>::infinity();

struct Ray {
    Ray()
        : origin()
        , direction()
    {}

    Ray(glm::vec3 origin, glm::vec3 direction)
        : origin(origin)
        , direction(direction)
    {}

    glm::vec3 origin;

    /**
     * Expected to be normalized
     */
    glm::vec3 direction;
    float intersection_distance = 1e30f;

    glm::vec3 at(float t) const { return origin + direction * t; }
};

class Interval {
public:
    float min, max;

    Interval()
        : min(+infinity)
        , max(-infinity)
    {
    }

    Interval(float min, float max)
        : min(min)
        , max(max)
    {
    }

    float size() const { return max - min; }
    bool contains(float x) const { return min <= x && x <= max; }
    bool surrounds(float x) const { return min < x && x < max; }

    float clamp(float x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    //static const Interval empty, universe;
};

struct Triangle {
  glm::vec3 v0;
  glm::vec3 v1;
  glm::vec3 v2;

  template <class T0, class T1, class T2>
  Triangle(T0&& _v0, T1&& _v1, T2&& _v2)
    : v0(std::forward<T0>(_v0)), v1(std::forward<T1>(_v1)), v2(std::forward<T2>(_v2)) {
    static_assert(std::is_same_v<std::decay_t<T0>, glm::vec3>, "T0 must be glm::vec3");
    static_assert(std::is_same_v<std::decay_t<T1>, glm::vec3>, "T1 must be glm::vec3");
    static_assert(std::is_same_v<std::decay_t<T2>, glm::vec3>, "T2 must be glm::vec3");
  }

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

//const Interval Interval::empty = Interval(+infinity, -infinity);
//const Interval Interval::universe = Interval(-infinity, +infinity);

}  // namespace Common
