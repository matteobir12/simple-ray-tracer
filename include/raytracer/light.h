#pragma once

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

namespace RayTracer {

    struct PointLight {
        glm::vec3 position;
        float intensity;
        glm::vec3 color;
        float pad1;
    
        PointLight(const glm::vec3& position, const glm::vec3& color, float intensity)
            : position(position), color(color), intensity(intensity), pad1(0.0) {}
    
    };

} // namespace RayTracer