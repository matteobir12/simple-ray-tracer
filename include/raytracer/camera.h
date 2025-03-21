#pragma once

#include <glm/vec3.hpp>
#include <glm/gtc/constants.hpp>

#include "common/types.h"
#include "common/utils.h"
#include "raytracer_types.h"
#include "world.h"
#include "light.h"

namespace RayTracer {

struct CameraSettings{
	float		aspect = 1.0;
	uint		width = 100;
	uint		samplesPerPixel = 10;
	uint		maxDepth = 10;
	float		vFov = 90;
	Point3		origin = Point3(0, 0, 0);
	Point3		lookAt = Point3(0, 0, -1);
	glm::vec3	vUp = glm::vec3(0, 1, 0);
	float		defocusAngle = 0;
	float		focusDist = 10;
};

class Camera
{
public:
	Camera(CameraSettings settings)
		: cameraSettings(settings)
		, width(0)
		, height(0)
		, pixelSamplesScale(0.0f)
		, center()
		, pixel00Loc()
		, pixelDeltaU()
		, pixelDeltaV()
		, u()
		, v()
		, w()
		, defocusDiskU()
		, defocusDiskV()
	{}

	void Initialize();
	Common::Ray GetRay(uint i, uint j);
	Color RayColor(const Common::Ray& r, uint depth, World& world, const PointLight& light) const;

	inline uint getWidth() const { return width; }
	inline uint getHeight() const { return height; }
	inline float getPixelSamplesScale() const { return pixelSamplesScale; }
	inline const CameraSettings& getSettings() const{ return cameraSettings; }

private:
	CameraSettings	cameraSettings;
	uint			width;
	uint			height;
	float			pixelSamplesScale;
	Point3			center;
	Point3			pixel00Loc;
	glm::vec3		pixelDeltaU;
	glm::vec3		pixelDeltaV;
	glm::vec3		u, v, w; //Camera basis vectors
	glm::vec3		defocusDiskU;
	glm::vec3		defocusDiskV;

	glm::vec3 SampleSquare() const;
	Point3 defocusDiskSample() const;
};
}