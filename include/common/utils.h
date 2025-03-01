#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtx/norm.hpp"

namespace Common {

inline float pi() {
	return glm::pi<float>();
}

inline float degToRad(float deg) {
	return glm::radians(deg);
}

inline glm::vec3 unitVector(const glm::vec3& v) {
	return v / glm::length(v);
}

inline float randomFloat() {
	return std::rand() / (RAND_MAX + 1.0f);
}

inline float randomFloat(float min, float max) {
	return min + (max - min) * randomFloat();
}

inline glm::vec3 randomVec3(float min, float max) {
	return glm::vec3(randomFloat(min, max), randomFloat(min, max), randomFloat(min, max));
}

inline glm::vec3 randomInUnitDisk() {
	while (true)
	{
		auto p = glm::vec3(randomFloat (-1, 1), randomFloat(-1, 1), 0);
		if (glm::length2(p) < 1)
			return p;
	}
}

inline glm::vec3 randomUnitVector() {
	while (true)
	{
		auto p = randomVec3(-1, 1);
		auto lensq = glm::length2(p);
		if (1e-160 < lensq && lensq <= 1)
			return p / std::sqrt(lensq);
	}
}

inline glm::vec3 randomOnHemisphere(const glm::vec3& normal) {
	glm::vec3 onSphere = randomUnitVector();
	if (glm::dot(onSphere, normal) > 0.0)
		return onSphere;
	else
		return -onSphere;
}

inline float linearToGamma(float linearComponent) {
	if (linearComponent > 0)
		return std::sqrt(linearComponent);

	return 0.0f;
}
}
