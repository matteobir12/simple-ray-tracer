#pragma once

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

namespace RayTracer {

    struct PointLight {
        glm::vec3 position;
        float pad0;
        glm::vec3 color;
        float pad1;
    
        PointLight(const glm::vec3& position, const glm::vec3& color)
            : position(glm::normalize(position)), color(color), pad0(0.0), pad1(0.0) {}
    
    };

} // namespace RayTracer