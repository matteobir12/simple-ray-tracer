#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <glm/gtx/norm.hpp>
#include <memory>

#include "common/types.h"

namespace RayTracer {

using uint = Common::uint;
using Point3 = glm::vec3;
using Color = glm::vec3;

class HitRecord
{
public:
	Point3 p;
	glm::vec3 normal;
	//std::shared_ptr<Material> mat;
	float t;
	bool frontFace;

	void setFaceNormal(const Common::Ray& r, const glm::vec3& outwardNormal)
	{
		frontFace = glm::dot(r.direction, outwardNormal) < 0;
		normal = frontFace ? outwardNormal : -outwardNormal;
	}
};

class Hittable {
public:
	virtual ~Hittable() = default;
	virtual bool Hit(const Common::Ray& r, const Common::Interval& interval, HitRecord& rec) = 0;
};

class Sphere : public Hittable {
public:
	glm::vec3 center;
	float radius;

	Sphere(const glm::vec3& center, float radius)
		: center(center)
		, radius(std::fmax(0.0f, radius))
	{
	}

	bool Hit(const Common::Ray& r, const Common::Interval& interval, HitRecord& rec) override {
		glm::vec3 oc = center - r.origin;
		auto a = glm::length2(r.direction);
		auto h = glm::dot(r.direction, oc);
		auto c = glm::length2(oc) - (radius * radius);
		auto discriminant = h * h - a * c;

		if (discriminant < 0) {
			return false;
		}

		auto sqrtd = std::sqrt(discriminant);

		auto root = (h - sqrtd) / a;
		if (!interval.surrounds(root))
		{
			root = (h + sqrtd) / a;
			if (!interval.surrounds(root))
				return false;
		}

		// This code gets the material properties of the hit sphere. Leaving out for now. Will likely be in the BRDF instead
		rec.t = root;
		rec.p = r.at(rec.t);
		glm::vec3 outwardNormal = (rec.p - center) / radius;
		rec.setFaceNormal(r, outwardNormal);
		//rec.mat = mat;

		return true;
	}
};

//TODO move Triangle here too
}