#include "raytracer/camera.h"
#include "raytracer/light.h"

namespace RayTracer {

void Camera::Initialize() {
	width = cameraSettings.width;
	height = int(cameraSettings.width / cameraSettings.aspect);
	height = (height < 1) ? 1 : height;

	pixelSamplesScale = 1.0f / cameraSettings.samplesPerPixel;

	center = cameraSettings.origin;

	// Camera Values
	auto theta = Common::degToRad(cameraSettings.vFov);
	auto h = std::tan(theta / 2);
	auto viewHeight = 2 * h * cameraSettings.focusDist;
	auto viewWidth = viewHeight * (float(cameraSettings.width) / height);

	w = Common::unitVector(cameraSettings.origin - cameraSettings.lookAt);
	u = Common::unitVector(glm::cross(cameraSettings.vUp, w));
	v = glm::cross(w, u);

	auto viewU = viewWidth * u;
	auto viewV = viewHeight * -v;

	pixelDeltaU = viewU / (float)cameraSettings.width;
	pixelDeltaV = viewV / (float)height;

	auto viewUpperLeft = center - (cameraSettings.focusDist * w) - viewU / 2.0f - viewV / 2.0f;
	pixel00Loc = viewUpperLeft + 0.5f * (pixelDeltaU + pixelDeltaV);

	// Camera Defocus
	auto defocusRadius = cameraSettings.focusDist = std::tan(Common::degToRad(cameraSettings.defocusAngle / 2));
	defocusDiskU = u * defocusRadius;
	defocusDiskV = v * defocusRadius;
}

Common::Ray Camera::GetRay(uint i, uint j) {
	auto offset = SampleSquare();
	auto pixelSample = pixel00Loc + ((i + offset.x) * pixelDeltaU) + ((j + offset.y) * pixelDeltaV);

	auto rayOrigin = (cameraSettings.defocusAngle <= 0) ? center : defocusDiskSample();
	auto rayDirection = pixelSample - rayOrigin;

	return Common::Ray(rayOrigin, rayDirection);
}

glm::vec3 Camera::SampleSquare() const {
	return glm::vec3(Common::randomFloat() - 0.5f, Common::randomFloat() - 0.5f, 0);
}

Point3 Camera::defocusDiskSample() const {
	auto p = Common::randomInUnitDisk();
	return center + (p[0] * defocusDiskU) + (p[1] * defocusDiskV);
}

Color Camera::RayColor(const Common::Ray& r, uint depth, World& world, const DirectionalLight& light) const {
	if (depth <= 0)
		return Color(0, 0, 0);

	HitRecord rec;
	if (world.CheckHit(r, Common::Interval(0.001f, Common::infinity), rec))
	{
		glm::vec3 lightDir = -light.direction;
		float diff = glm::max(glm::dot(rec.normal, lightDir), 0.0f);
		Color diffuse = diff * light.color;
		// This can be modified to accomodate for different materials by calling rec.mat->... when materials are implemented into HitRecord

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

}