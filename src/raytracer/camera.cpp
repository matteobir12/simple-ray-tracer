#include <iostream>
#include "raytracer/camera.h"
#include "raytracer/light.h"

namespace RayTracer
{

    void Camera::Initialize()
    {
        width = cameraSettings.width;
        height = int(cameraSettings.width / cameraSettings.aspect);
        height = (height < 1) ? 1 : height;

        pixelSamplesScale = 1.0f / cameraSettings.samplesPerPixel;

        UpdateCameraVectors();
    }

    Common::Ray Camera::GetRay(uint i, uint j)
    {
        auto offset = SampleSquare();
        auto pixelSample = pixel00Loc + ((i + offset.x) * pixelDeltaU) + ((j + offset.y) * pixelDeltaV);

        auto rayOrigin = (cameraSettings.defocusAngle <= 0) ? position : defocusDiskSample();
        auto rayDirection = pixelSample - rayOrigin;

        return Common::Ray(rayOrigin, rayDirection);
    }

    glm::vec3 Camera::SampleSquare() const
    {
        return glm::vec3(Common::randomFloat() - 0.5f, Common::randomFloat() - 0.5f, 0);
    }

    Point3 Camera::defocusDiskSample() const
    {
        auto p = Common::randomInUnitDisk();
        return position + (p[0] * defocusDiskU) + (p[1] * defocusDiskV);
    }

    Color Camera::RayColor(const Common::Ray &r, uint depth, World &world, const PointLight &light) const
    {
        if (depth <= 0)
            return Color(0, 0, 0);

        HitRecord rec;
        if (world.CheckHit(r, Common::Interval(0.001f, Common::infinity), rec))
        {
            glm::vec3 lightDir = -light.position;
            float diff = glm::max(glm::dot(rec.normal, lightDir), 0.0f);
            Color diffuse = diff * light.color;

            glm::vec3 viewDir = glm::normalize(-r.direction);
            glm::vec3 reflectDir = glm::reflect(-lightDir, rec.normal);
            float spec = glm::pow(glm::max(glm::dot(viewDir, reflectDir), 0.0f), 32);
            Color specular = spec * light.color;

            Color ambient = 0.1f * light.color;
            Color result = (ambient + diffuse + specular);

            glm::vec3 direction = Common::randomOnHemisphere(rec.normal);
            return result * RayColor(Common::Ray(rec.p, direction), depth - 1, world, light);
        }

        glm::vec3 unitDirection = glm::normalize(r.direction);
        auto a = 0.5f * (unitDirection.y + 1.0f);
        return (1.0f - a) * Color(1.0f, 1.0f, 1.0f) + a * Color(0.5f, 0.7f, 1.0f);
    }

    void Camera::MoveForward(float delta)
    {
        position += front * delta;
        UpdateCameraVectors();
    }

    void Camera::MoveBackward(float delta)
    {
        position -= front * delta;
        UpdateCameraVectors();
    }

    void Camera::MoveLeft(float delta)
    {
        position -= right * delta;
        UpdateCameraVectors();
    }

    void Camera::MoveRight(float delta)
    {
        position += right * delta;
        UpdateCameraVectors();
    }

    void Camera::MoveUp(float delta)
    {
        position += up * delta;
        UpdateCameraVectors();
    }

    void Camera::MoveDown(float delta)
    {
        position -= up * delta;
        UpdateCameraVectors();
    }

    void Camera::Rotate(float yawOffset, float pitchOffset)
    {
        yaw += yawOffset;
        pitch += pitchOffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        UpdateCameraVectors();
    }

    void Camera::UpdateCameraVectors()
    {
        // Calculate the new Front vector from yaw and pitch (spherical to cartesian)
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        // Make sure we're working with unit vectors
        front = glm::normalize(newFront);

        // Re-calculate Right and Up vectors
        // Use fixed world up vector to avoid drift
        glm::vec3 fixedWorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        right = glm::normalize(glm::cross(front, fixedWorldUp));
        up = glm::normalize(glm::cross(right, front));
    }

    void Camera::MoveAndRotate(float deltaTime, const glm::vec3 &moveDelta, const glm::vec2 &rotDelta, float speed)
    {
        // Apply rotation with better numerical stability
        if (glm::abs(rotDelta.x) > 0.0001f || glm::abs(rotDelta.y) > 0.0001f)
        {
            // Update yaw and pitch
            yaw += rotDelta.x;
            pitch += rotDelta.y;

            // Constrain pitch to prevent gimbal lock
            pitch = glm::clamp(pitch, -89.0f, 89.0f);

            // Normalize yaw to prevent floating-point drift issues
            // Keep yaw within -180 to 180 range
            while (yaw > 180.0f)
                yaw -= 360.0f;
            while (yaw < -180.0f)
                yaw += 360.0f;

            // Force recalculation of vectors
            UpdateCameraVectors();
        }

        // Only apply movement if there's actual movement input
        if (glm::length(moveDelta) > 0.0001f)
        {
            // Scale movement by delta time and speed
            float adjustedSpeed = speed * deltaTime;

            // Move along each axis
            position += front * moveDelta.z * adjustedSpeed;
            position += right * moveDelta.x * adjustedSpeed;
            position += up * moveDelta.y * adjustedSpeed;
        }

        // Periodically re-orthogonalize the camera basis vectors
        // This is crucial to prevent drift over time
        static int frameCounter = 0;
        if (++frameCounter % 120 == 0)
        {
            // Re-orthogonalize to prevent cumulative error
            front = glm::normalize(front);
            // Use fixed up vector (0,1,0) as reference to prevent drift
            glm::vec3 fixedWorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
            right = glm::normalize(glm::cross(front, fixedWorldUp));
            up = glm::normalize(glm::cross(right, front));
        }
    }

    void Camera::Reset()
    {
        // Reset to default values
        position = glm::vec3(0.0f, 1.0f, 4.0f); // Adjusted to better see the model
        cameraSettings.lookAt = glm::vec3(0.0f, 0.0f, 0.0f);

        // Reset angle values
        yaw = -90.0f; // Looking forward along negative Z
        pitch = 0.0f; // Looking straight ahead

        // Reset internal vectors
        front = glm::vec3(0.0f, 0.0f, -1.0f);
        right = glm::vec3(1.0f, 0.0f, 0.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);

        // Update all vectors and matrices
        UpdateCameraVectors();
        Initialize();

        // Debug output
        std::cout << "Camera reset: Position (" << position.x << ", "
                  << position.y << ", " << position.z << ")" << std::endl;
    }

}