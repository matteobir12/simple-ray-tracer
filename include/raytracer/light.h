#pragma once

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

namespace RayTracer {

    struct DirectionalLight {
        glm::vec3 direction;
        glm::vec3 color;
    
        DirectionalLight(const glm::vec3& direction, const glm::vec3& color) 
            : direction(glm::normalize(direction)), color(color) {}
    
    };

} // namespace RayTracer