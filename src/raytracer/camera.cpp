#include "raytracer/camera.h"

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

Color Camera::RayColor(const Common::Ray& r, uint depth, World& world) const {
	if (depth <= 0)
		return Color(0, 0, 0);

	HitRecord rec;
	if (world.CheckHit(r, Common::Interval(0.001f, Common::infinity), rec))
	{
		//Common::Ray scattered;
		//Color attenuation;

		// This is the original correct path tracer version of this
		// Will need to redo this once brdf is in place
		/*if (rec.mat->Scatter(r, rec, attenuation, scattered))
			return attenuation * RayColor(scattered, depth - 1, world);
		return Color(0, 0, 0);*/

		// This is basically a perfectly diffuse brdf, but doesn't divide by pi so needs to be fixed
		glm::vec3 direction = Common::randomOnHemisphere(rec.normal);
		return 0.5f * RayColor(Common::Ray(rec.p, direction), depth - 1, world);
	}

	glm::vec3 direction = glm::normalize(r.direction);
	auto a = 0.5f * (direction.y + 1.0f);
	return (1.0f - a) * Color(1.0f, 1.0f, 1.0f) + a * Color(0.5f, 0.7f, 1.0f);
}

}