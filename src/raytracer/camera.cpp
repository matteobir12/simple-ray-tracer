#include "raytracer/camera.h"
#include "raytracer/light.h"

namespace RayTracer {

void Camera::Initialize() {
    width = cameraSettings.width;
    height = int(cameraSettings.width / cameraSettings.aspect);
    height = (height < 1) ? 1 : height;

    pixelSamplesScale = 1.0f / cameraSettings.samplesPerPixel;

    UpdateCameraVectors();
}

Common::Ray Camera::GetRay(uint i, uint j) {
    auto offset = SampleSquare();
    auto pixelSample = pixel00Loc + ((i + offset.x) * pixelDeltaU) + ((j + offset.y) * pixelDeltaV);

    auto rayOrigin = (cameraSettings.defocusAngle <= 0) ? position : defocusDiskSample();
    auto rayDirection = pixelSample - rayOrigin;

    return Common::Ray(rayOrigin, rayDirection);
}

glm::vec3 Camera::SampleSquare() const {
    return glm::vec3(Common::randomFloat() - 0.5f, Common::randomFloat() - 0.5f, 0);
}

Point3 Camera::defocusDiskSample() const {
    auto p = Common::randomInUnitDisk();
    return position + (p[0] * defocusDiskU) + (p[1] * defocusDiskV);
}

Color Camera::RayColor(const Common::Ray& r, uint depth, World& world, const PointLight& light) const {
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

void Camera::MoveForward(float delta) {
    position += front * delta;
    UpdateCameraVectors();
}

void Camera::MoveBackward(float delta) {
    position -= front * delta;
    UpdateCameraVectors();
}

void Camera::MoveLeft(float delta) {
    position -= right * delta;
    UpdateCameraVectors();
}

void Camera::MoveRight(float delta) {
    position += right * delta;
    UpdateCameraVectors();
}

void Camera::MoveUp(float delta) {
    position += up * delta;
    UpdateCameraVectors();
}

void Camera::MoveDown(float delta) {
    position -= up * delta;
    UpdateCameraVectors();
}

void Camera::Rotate(float yawOffset, float pitchOffset) {
    yaw += yawOffset;
    pitch += pitchOffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    UpdateCameraVectors();
}

void Camera::UpdateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    right = glm::normalize(glm::cross(front, up));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::MoveAndRotate(float deltaTime, const glm::vec3& movementDelta, const glm::vec2& rotationDelta) {
    float movementSpeed = 0.5f;
    position += movementDelta.x * right * deltaTime * movementSpeed; 
    position += movementDelta.y * up * deltaTime * movementSpeed;     
    position += movementDelta.z * front * deltaTime * movementSpeed; 

    yaw += rotationDelta.x;
    pitch += rotationDelta.y;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    UpdateCameraVectors();
}

}